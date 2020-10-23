#include "net_session.h"
#include <cstdlib>
#include <stdio.h>

#include "net_packet.h"
#include "con_socket.h"

Net_session::Net_session(uint32_t id, int max_clients, Con_socket* con_socket, int udp_port) : _id(id), _max_clients(max_clients), _on_pos(nullptr), _con_socket(con_socket) {
    int p = udp_port;

    _world = std::make_shared<World_instance>();

    _pos_socket = std::make_shared<Pos_socket>(p);

    if (_pos_socket->init()) {
        printf("[NET-SESSION][OK][id: %d][POS-UDP: %d]\n", _id, p);
    }
    else {
        printf("[NET-SESSION][FAIL][id: %d][POS-UDP: %d]\n", _id, p);
    }
    
    _pos_socket->set_on_data([&](const std::vector<uint8_t>& data) {
        on_udp_data(data);
    });
}

Net_session::~Net_session() {}

void Net_session::on_udp_data(const std::vector<uint8_t>& data) {
    MsgType type = (MsgType)data[0];

    switch(type) {
        case MsgType::PlayerPos:
            printf("[NET-SESSION][UDP-PLAYER-POS]");
    }
}

void Net_session::disconnect(uint16_t entity_id) {
    for (auto c : _clients) {
        
    }
}

void Net_session::handle_inc_pos(const std::vector<uint8_t>& data) {
    Net_pos pos;

    pos.from_buffer(data);

    if (_on_pos != nullptr) {
        _on_pos(pos);
    }
}

void Net_session::set_on_pos(std::function<void(const Net_pos&)> func) {
    _on_pos = func;
}

bool Net_session::add_client(std::shared_ptr<Net_client> client) {

    if (_clients.size() >= _max_clients) {
        return false;
    }

    _clients.push_back(client);

    return true;
}

bool Net_session::send_config() {
    Net_group_config config;

    strcpy(config.hostname, "localhost");
    config.group_id = _id;
    config.pos_port = _pos_socket->get_port();
    

    std::vector<uint8_t> data;
    data.resize(sizeof(Net_group_config));
    config.set_buffer(data, 0);

    for (auto client : _clients) {
        _con_socket->sendto_client(client, data);
    }

    return true;
}

bool Net_session::send_pos() {
    return true;
}

int Net_session::read() {
    return _pos_socket->read();
}

