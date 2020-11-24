#include "transform_entity.h"

#include <cmath>

Transform_entity::Transform_entity(uint16_t id) : entity_id(id), pos(0,0,0), rot(0,0,0,1) {
    raw_pos_data.resize(7 + 3 * sizeof(uint16_t));
}

Transform_entity::~Transform_entity() {}

void Transform_entity::set_inc_pos(const Net_pos& netpos) {
    memcpy(&raw_pos_data[0], netpos.pos, sizeof(uint16_t) * 3);
    memcpy(&raw_pos_data[sizeof(uint16_t) * 3], netpos.rot, 7);

    pos.x = (float)netpos.pos[0] / 100.0f;
    pos.y = (float)netpos.pos[1] / 100.0f;
    pos.z = (float)netpos.pos[2] / 100.0f;

    rot = netpos.to_quat();
}

void Transform_entity::set_out_pos(Net_pos& netpos) {
    netpos.pos[0] = (int16_t)(pos.x * 100.0f);
    netpos.pos[1] = (int16_t)(pos.y * 100.0f);
    netpos.pos[2] = (int16_t)(pos.z * 100.0f);
}

void Transform_entity::fill_data(std::vector<uint8_t>& data, uint32_t offset) {
    memcpy(&data[offset], &raw_pos_data[0], sizeof(uint16_t) * 3 + 7);
}

void Transform_entity::quat_to_data(std::vector<uint8_t>& data, uint32_t offset) {
    uint8_t maxIndex = 0;
    float maxValue = FLT_MIN;
    float sign = 1.0f;

    // Determine the index of the largest (absolute value) element in the Quaternion.
    // We will transmit only the three smallest elements, and reconstruct the largest
    // element during decoding. 
    for (int i = 0; i < 4; i++)
    {
        float element = rot[i];
        float ab = abs(rot[i]);
        if (ab > maxValue)
        {
            // We don't need to explicitly transmit the sign bit of the omitted element because you 
            // can make the omitted element always positive by negating the entire quaternion if 
            // the omitted element is negative (in quaternion space (x,y,z,w) and (-x,-y,-z,-w) 
            // represent the same rotation.), but we need to keep track of the sign for use below.
            sign = (element < 0) ? -1 : 1;

            // Keep track of the index of the largest element
            maxIndex = i;
            maxValue = ab;
        }
    }

    // If the maximum value is approximately 1f (such as Quaternion.identity [0,0,0,1]), then we can 
    // reduce storage even further due to the fact that all other fields must be 0f by definition, so 
    // we only need to send the index of the largest field.
    if (abs(maxValue - 1.0f) < 0.01f)
    {
        // Again, don't need to transmit the sign since in quaternion space (x,y,z,w) and (-x,-y,-z,-w) 
        // represent the same rotation. We only need to send the index of the single element whose value
        // is 1f in order to recreate an equivalent rotation on the receiver.
        data[offset] = (uint8_t)maxIndex + 4;

        return;
    }

    short a = (short)0;
    short b = (short)0;
    short c = (short)0;

    if (maxIndex == 0)
    {
        a = (short)(rot.y * sign * FLOAT_PRECISION_MULT);
        b = (short)(rot.z * sign * FLOAT_PRECISION_MULT);
        c = (short)(rot.w * sign * FLOAT_PRECISION_MULT);
    }
    else if (maxIndex == 1)
    {
        a = (short)(rot.x * sign * FLOAT_PRECISION_MULT);
        b = (short)(rot.z * sign * FLOAT_PRECISION_MULT);
        c = (short)(rot.w * sign * FLOAT_PRECISION_MULT);
    }
    else if (maxIndex == 2)
    {
        a = (short)(rot.x * sign * FLOAT_PRECISION_MULT);
        b = (short)(rot.y * sign * FLOAT_PRECISION_MULT);
        c = (short)(rot.w * sign * FLOAT_PRECISION_MULT);
    }
    else
    {
        a = (short)(rot.x * sign * FLOAT_PRECISION_MULT);
        b = (short)(rot.y * sign * FLOAT_PRECISION_MULT);
        c = (short)(rot.z * sign * FLOAT_PRECISION_MULT);
    }

    data[offset] = maxIndex;
    memcpy(&data[(size_t)offset + 1], &a, sizeof(uint16_t));
    memcpy(&data[(size_t)offset + 3], &b, sizeof(uint16_t));
    memcpy(&data[(size_t)offset + 5], &c, sizeof(uint16_t));
}