#pragma once

#include <stdint.h>

#include "net_packet.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct Transform_entity {

    uint16_t entity_id;
    
    glm::vec3 pos;
    glm::quat rot;

    Transform_entity(uint16_t id);
    virtual ~Transform_entity();

    void set_inc_pos(const Net_pos& pos);
    void set_out_pos(Net_pos& netpos);
};