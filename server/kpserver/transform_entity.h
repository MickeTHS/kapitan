#pragma once

#include <stdint.h>

#include "net_packet.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct Transform_entity {

    uint16_t entity_id;
    
    glm::vec3 pos;
    glm::quat rot;

    std::vector<uint8_t> raw_pos_data;

    Transform_entity(uint16_t id);
    
    virtual ~Transform_entity();

    void set_inc_pos(const Net_pos& pos);

    void set_out_pos(Net_pos& netpos);

    void fill_data(std::vector<uint8_t>& data, uint32_t offset);

    void quat_to_data(std::vector<uint8_t>& data, uint32_t offset);
};