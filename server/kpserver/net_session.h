#pragma once

#include <memory>
#include <vector>
#include <stdint.h>
#include <functional>
#include <chrono>

#include "net_client.h"
#include "udp_server.h"
#include "net_packet.h"
#include "world_instance.h"
#include "hash.h"

struct Tcp_server;

/// a group is a collection of clients that will be playing a private game
/// that once it has been started, only these clients will be able to 
/// communicate among each other
struct Net_session {
    Net_session(uint32_t id, int max_clients, Tcp_server* tcp_server, int udp_port);
    virtual ~Net_session();

    bool add_client(std::shared_ptr<Net_client> client);
    int32_t on_tcp_data(const std::vector<uint8_t>& data, uint32_t offset, int32_t len);
    int read();
    void set_on_pos(std::function<void(const Net_pos&)> func);
    void disconnect(uint16_t entity_id);
    void generate_session_code();
    void keepalive();
    void disconnect(std::shared_ptr<Net_client> client);

    char session_code[16];
    mmh::Hash_key session_code_hash;
private:
    void handle_inc_pos(const std::vector<uint8_t>& data);
    void on_udp_data(const std::vector<uint8_t>& data);

    std::vector<std::shared_ptr<Net_client>> _clients;
    uint32_t _id;
    int _max_clients;

    Tcp_server* _tcp_server;

    std::shared_ptr<Udp_server> _udp_server;
    std::shared_ptr<World_instance> _world;

    std::function<void(const Net_pos&)> _on_pos;

    std::chrono::steady_clock::time_point _last_keepalive;
};