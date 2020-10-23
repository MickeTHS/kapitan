#pragma once

#include <memory>
#include <vector>
#include <stdint.h>
#include <functional>

#include "net_client.h"
#include "pos_socket.h"
#include "net_packet.h"
#include "world_instance.h"

struct Con_socket;

/// a group is a collection of clients that will be playing a private game
/// that once it has been started, only these clients will be able to 
/// communicate among each other
struct Net_session {
    Net_session(uint32_t id, int max_clients, Con_socket* con_socket, int udp_port);
    virtual ~Net_session();

    bool add_client(std::shared_ptr<Net_client> client);
    bool send_config();
    bool send_pos();
    int read();
    void set_on_pos(std::function<void(const Net_pos&)> func);
    void disconnect(uint16_t entity_id);
private:
    void handle_inc_pos(const std::vector<uint8_t>& data);
    void on_udp_data(const std::vector<uint8_t>& data);

    std::vector<std::shared_ptr<Net_client>> _clients;
    uint32_t _id;
    int _max_clients;

    Con_socket* _con_socket;

    std::shared_ptr<Pos_socket> _pos_socket;
    std::shared_ptr<World_instance> _world;

    std::function<void(const Net_pos&)> _on_pos;
};