#include "world_instance.h"

#include "net_session_rules.h"

World_instance::World_instance() {
    
}

World_instance::~World_instance() {

}

void World_instance::on_player_pos(const Net_pos& pos) {
    /*if (entities[pos.entity] == nullptr) {
        return;
    }

    entities[pos.entity]->set_inc_pos(pos);*/

    
}

void World_instance::add_player(uint16_t player_id) {
    if (entities.find(player_id) == entities.end()) {
        // not found, add
        entities[player_id] = std::make_shared<Transform_entity>(player_id);
    }
}