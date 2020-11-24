#include "net_session_player.h"

#include <string>
#include <stdlib.h>
#include <time.h>
#include "net_client.h"


Net_session_player::Net_session_player() {
    net_player_short_id = 0;
    entity_session_id = 0;
    net_session_id = 0;
    net_player_id = 0;
    net_client_id = 0;
    node_slave_id = 0;
    client_connection = 0;

    is_set = false;
}

Net_session_player::~Net_session_player() {

}


void Net_session_player::assign(const Net_session_player& ref) {
    net_player_short_id = ref.net_player_short_id;
    net_client_id = ref.net_client_id;
    client_connection = ref.client_connection;

    is_set = true;
}

void Net_session_player::assign(uint8_t net_player_short_id_, uint32_t client_id_, Net_client* client) {
    net_player_short_id = net_player_short_id_;
    net_client_id = client_id_;
    client_connection = client;

    is_set = true;
}

bool Net_session_player::is_valid_for_play() const {
    if (entity_session_id != 0 && net_session_id != 0 && net_player_id != 0 && net_client_id != 0) {
        return true;
    }

    return false;
}

void Net_session_player::reset() {
    net_player_short_id = 0;
    entity_session_id = 0;
    net_session_id = 0;
    net_player_id = 0;
    net_client_id = 0;
    node_slave_id = 0;
    client_connection = 0;

    is_set = false;
}