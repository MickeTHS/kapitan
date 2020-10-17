#pragma once

#include <memory>
#include <vector>
#include <stdint.h>

#include "net_client.h"
#include "pos_socket.h"

struct Con_socket;

/// a group is a collection of clients that will be playing a private game
/// that once it has been started, only these clients will be able to 
/// communicate among each other
struct Net_group {
    Net_group(uint32_t id, int max_clients, Con_socket* con_socket);
    virtual ~Net_group();

    bool add_client(std::shared_ptr<Net_client> client);
    bool send_config();

private:
    std::vector<std::shared_ptr<Net_client>> _clients;
    uint32_t _id;
    int _max_clients;

    Con_socket* _con_socket;

    std::shared_ptr<Pos_socket> _pos_socket;
};