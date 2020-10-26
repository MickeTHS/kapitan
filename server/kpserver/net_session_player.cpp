#include "net_session_player.h"

#include <string>
#include <stdlib.h>
#include <time.h>


Net_session_player::Net_session_player() {
    entity_session_id = 0;
    net_session_id = 0;
    net_player_id = 0;
    net_client_id = 0;
    node_slave_id = 0;
}

Net_session_player::~Net_session_player() {

}


bool Net_session_player::is_valid_for_play() const {
    if (entity_session_id != 0 && net_session_id != 0 && net_player_id != 0 && net_client_id != 0) {
        return true;
    }

    return false;
}