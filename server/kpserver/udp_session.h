#pragma once

#include "net_packet.h"
#include "net_session.h"
#include "world_player_entity.h"
#include <memory>
#include <chrono>
#include <vector>
#include <unordered_map>
#include <stdint.h>

///
/// Keeps track of the player positions in a session
/// must update per tick to send out the latest position updates
///
struct Udp_session {
    Udp_session(std::shared_ptr<Net_session> net_session, int disconnect_player_after_milliseconds_inactivity);

    void add_player(std::shared_ptr<World_player_entity> player);
    void remove_player(std::shared_ptr<World_player_entity> player);
    void update();
private:
    void on_pos_inc_data(const Net_pos& pos);

    std::shared_ptr<Net_session> _net_session;

    float _tickrate; // updates per second
    long _tick_microseconds;
    std::chrono::steady_clock::time_point _last_tick;

    int _pos_check;
    int _disconnect_player_after_milliseconds_inactivity;

    std::unordered_map<uint16_t, std::shared_ptr<World_player_entity>> _players;
};