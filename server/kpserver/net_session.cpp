#include "net_session.h"
#include "net_packet.h"
#include "tcp_server.h"
#include "udp_server.h"
#include "net_client.h"

#include <cstdlib>
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <time.h>


Net_session::Net_session(uint32_t id, int max_players, Tcp_server* tcp_server, Udp_server* udp_server)
    :   _id(id), 
        _max_players(max_players), 
        _tcp_server(tcp_server),
        _udp_server(udp_server) {
    
    _players.resize(max_players);
    
    memset(session_code, 0, 16);
    generate_session_code();
}

Net_session::~Net_session() {}

void Net_session::keepalive() {
    _last_keepalive = std::chrono::high_resolution_clock::now();
}

bool Net_session::is_full() const {
    uint16_t num_players = 0;

    for (auto& player : _players) {
        if (player.is_set) {
            num_players++;
        }
    }

    return num_players == _max_players;
}

void Net_session::disconnect(uint32_t client_id) {
    for (int i = 0; i < _players.size(); ++i) {
        if (_players[i].net_client_id == client_id) {
            _players.erase(_players.begin() + i);
            return;
        }
    }
}

void Net_session::generate_session_code() {
    char alfa[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ\0";
    int len = strlen(alfa);

    srand(time(NULL));

    for (int i = 0; i < 8; ++i) {
        session_code[i] = alfa[rand() % len];
    }

    session_code[8] = '\0';

    session_code_hash = mmh::Hash_key(session_code);
}

void Net_session::broadcast(const std::vector<uint8_t>& data, uint32_t len) {
    
}

void Net_session::on_udp_data(const std::vector<uint8_t>& data, int32_t data_len) {
    /*MsgType type = (MsgType)data[0];

    switch(type) {
        case MsgType::PlayerPos:
            printf("[NET-SESSION][UDP-PLAYER-POS]");
    }*/
}

void Net_session::handle_inc_pos(const std::vector<uint8_t>& data) {
    Net_pos pos(data, 0);

    if (_on_pos != nullptr) {
        _on_pos(pos);
    }
}

int32_t Net_session::on_tcp_data(const std::vector<uint8_t>& data, uint32_t offset, int32_t len) {
    // when a packet was unable to be parsed by the
    // net_slave it will enter here to be processed

    // return the amount of data read
    return 0;
}

void Net_session::set_on_pos(std::function<void(const Net_pos&)> func) {
    _on_pos = func;
}

bool Net_session::add_player(Net_client* client) {
    if (_num_players == _max_players) {
        return false; // its full
    }

    _players[_num_players].assign(_num_players, client->info.client_id, client);
    _num_players++;

    return true;
}

int Net_session::read() {
    return _udp_server->read();
}

