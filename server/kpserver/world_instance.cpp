#include "world_instance.h"

#include "net_session_rules.h"

World_instance::World_instance() : _time(0) {
    data_transforms.num_items = 0;
    data_transforms.num_players = 0;

    scene.on_item_states_updated = [&](uint16_t id, uint8_t states) {
        _on_item_states_updated(id, states);
    };
}

World_instance::~World_instance() {

}

void World_instance::on_player_pos(const Net_pos& pos) {
    
    _players_by_index[pos.player_index]->pos = pos.to_vec3();
    _players_by_index[pos.player_index]->rot = pos.to_quat();

}

/// <summary>
/// Starting the game
/// so we set the correct size of the tranform vectors
/// </summary>
void World_instance::start() {
    data_transforms.num_players = _players.size();
    data_transforms.player_transforms.resize((size_t)data_transforms.num_players * (3 + 7));
}

void World_instance::add_player(uint16_t player_id) {
    for (int i = 0; i < _players.size(); ++i) {
        if (_players[i]->entity_id == player_id) {
            return;
        }
    }

    _players.push_back(std::make_unique<Transform_entity>(player_id));
    _players_by_index[_players.size()-1] = _players[_players.size()-1].get();
}

void World_instance::update(double delta) {
    _time += delta;
}

double World_instance::get_time() const {
    return _time;
}

void World_instance::fill_transform() {
    int packet_size = (sizeof(uint16_t) * 3 + 7);
    uint32_t offset = 0;

    for (int i = 0; i < _players.size(); ++i) {
        _players[i]->fill_data(data_transforms.player_transforms, offset);
        offset += packet_size;
    }
}

bool World_instance::set_item_state(const Net_player_set_item_state_request& request, Net_player_set_item_state_response& resp) {
    
    
    if (!scene.set_item_state(request.id, request.state, request.on)) {
        return false;
    }
     return true;
}

void World_instance::set_item_snapshot_data(uint8_t* data) {
    //item_states.set_data(data);
}

void World_instance::set_on_item_states_updated(std::function<void(uint16_t id, uint8_t state)> func) {
    _on_item_states_updated = func;
}