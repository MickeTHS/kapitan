#include "world_player_entity.h"

World_player_entity::World_player_entity() {
    session_id = 0;
    session_entity_id = 0;
    net_client_id = 0;
    
    last_pos = glm::vec3(0,0,0);
    last_rot = glm::quat(0,0,0,1);

    memset(username, 0, 64);
    memset(avatar, 0, sizeof(uint16_t) * 32);
    
    last_input_ts = std::chrono::high_resolution_clock::now();
}

World_player_entity::~World_player_entity() {}