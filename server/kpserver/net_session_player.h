#pragma once

#include <stdint.h>
#include <cstdlib>
#include "hash.h"


struct Net_session_player {
    Net_session_player();
    virtual ~Net_session_player();

    uint16_t entity_session_id;
    uint32_t net_session_id;
    uint32_t net_player_id;
    uint32_t net_client_id;
    uint32_t node_slave_id;

    bool is_valid_for_play() const;
};