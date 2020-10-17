#include "net_group.h"
#include <cstdlib>
#include <stdio.h>

#include "net_packet.h"
#include "con_socket.h"

#define POS_PORT_START 2048

Net_group::Net_group(uint32_t id, int max_clients, Con_socket* con_socket) : _id(id), _max_clients(max_clients) {
    _con_socket = con_socket;

    int rr = rand() % 3000;

    int p = POS_PORT_START + rr;

    printf("Random port is: %d\n", p);

    _pos_socket = std::make_shared<Pos_socket>(p);

    if (_pos_socket->init()) {
        printf("Initialized POS socket '%d' for group '%d'\n", p, _id);
    }
    else {
        printf("ERROR: Init POS socket '%d' for group '%d'\n", p, _id);
    }
}

Net_group::~Net_group() {}

bool Net_group::add_client(std::shared_ptr<Net_client> client) {

    if (_clients.size() >= _max_clients) {
        return false;
    }

    _clients.push_back(client);

    return true;
}

bool Net_group::send_config() {
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

