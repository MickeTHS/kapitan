#pragma once

#include <stdint.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <chrono>

struct World_player_entity {
    uint32_t net_client_id;
    uint16_t entity_id;

    glm::vec3 last_pos;
    glm::quat last_rot;

    char username[64];
    char avatar[32];

    std::chrono::steady_clock::time_point last_input_ts;
};