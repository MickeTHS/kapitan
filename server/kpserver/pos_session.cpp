#include "pos_session.h"

Pos_session::Pos_session(std::shared_ptr<Net_session> net_session, int disconnect_player_after_milliseconds_inactivity) {
    _net_session = net_session;
    _disconnect_player_after_milliseconds_inactivity = disconnect_player_after_milliseconds_inactivity;

    _net_session->set_on_pos([&](const Net_pos& pos) {
        on_pos_inc_data(pos);
    });

    _tickrate = 20.0f; // 20 packets per second
    _tick_microseconds = (1.0f / _tickrate) * 1000000;
    _last_tick = std::chrono::high_resolution_clock::now();
}

void Pos_session::on_pos_inc_data(const Net_pos& pos) {
    // on pos data received
    auto player = _players.find(pos.entity);

    if (player == _players.end()) {
        // did not find player
        return;
    }

    // update the player position
    player->second->last_pos.x = ((float)pos.pos[0] / 100.0f);
    player->second->last_pos.y = ((float)pos.pos[1] / 100.0f);
    player->second->last_pos.z = ((float)pos.pos[2] / 100.0f);

    // update the player rotation
    player->second->last_rot.x = ((float)pos.rot[0] / 100.0f);
    player->second->last_rot.y = ((float)pos.rot[1] / 100.0f);
    player->second->last_rot.z = ((float)pos.rot[2] / 100.0f);
    player->second->last_rot.w = ((float)pos.rot[3] / 100.0f);

    // store the timestamp of the last message
    player->second->last_input_ts = std::chrono::high_resolution_clock::now();
}

void Pos_session::add_player(std::shared_ptr<World_player_entity> player) {
    _players[player->entity_id] = player;
}

void Pos_session::remove_player(std::shared_ptr<World_player_entity> player) {
    _players.erase(_players.find(player->entity_id));
}

void Pos_session::update() {
    auto now = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - _last_tick);

    if (duration.count() < _tick_microseconds) {
        // not time yet
        return;
    }

    _last_tick = now;

    _pos_check++;

    if (_pos_check > _tickrate * 10) {
        // check the player session statuses every 2 seconds
        _pos_check = 0;

        for (auto player : _players) {
            auto time_since_last_input = std::chrono::duration_cast<std::chrono::milliseconds>(now - player.second->last_input_ts);

            if (time_since_last_input.count() >= _disconnect_player_after_milliseconds_inactivity) {
                // disconnect player
                _net_session->disconnect(player.second->entity_id);
            }
        }
    }
}