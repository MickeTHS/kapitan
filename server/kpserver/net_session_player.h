#pragma once

#include <stdint.h>
#include <cstdlib>
#include <memory>
#include <vector>

#include "hash.h"

struct Net_client_info;
struct Net_client;

struct Net_session_player {
    Net_session_player();
    virtual ~Net_session_player();

    void assign(uint8_t net_player_short_id_, uint32_t client_id_, Net_client* info);

    bool is_valid_for_play() const;

    void reset();
    
    uint8_t net_player_short_id;
    uint16_t entity_session_id;
    uint32_t net_session_id;
    uint32_t net_player_id;
    uint32_t net_client_id;
    uint32_t node_slave_id;

    Net_client* client_connection;

    bool is_set;
};