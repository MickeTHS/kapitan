#include "transform_entity.h"

Transform_entity::Transform_entity(uint16_t id) : entity_id(id), pos(0,0,0), rot(0,0,0,1) {}

Transform_entity::~Transform_entity() {}

void Transform_entity::set_inc_pos(const Net_pos& netpos) {
    pos.x = (float)netpos.pos[0] / 100.0f;
    pos.y = (float)netpos.pos[1] / 100.0f;
    pos.z = (float)netpos.pos[2] / 100.0f;
}

void Transform_entity::set_out_pos(Net_pos& netpos) {
    netpos.pos[0] = (int16_t)(pos.x * 100.0f);
    netpos.pos[1] = (int16_t)(pos.y * 100.0f);
    netpos.pos[2] = (int16_t)(pos.z * 100.0f);
}