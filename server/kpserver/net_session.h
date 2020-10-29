#pragma once

#include <memory>
#include <vector>
#include <stdint.h>
#include <functional>
#include <chrono>
#include <unordered_map>

#include "net_packet.h"
#include "world_instance.h"
#include "hash.h"
#include "net_session_player.h"

struct Tcp_server;
struct Udp_server;
struct Net_client_info;
struct Net_client;

/// a group is a collection of clients that will be playing a private game
/// that once it has been started, only these clients will be able to 
/// communicate among each other
struct Net_session {
    Net_session(uint32_t id, uint8_t max_players, uint32_t keepalive_seconds, Tcp_server* tcp_server, Udp_server* udp_server);

    virtual ~Net_session();

    bool add_player_and_broadcast(Net_client* info, bool broadcast, bool is_owner);

    int32_t on_tcp_data(const std::vector<uint8_t>& data, uint32_t offset, int32_t len);

    uint32_t get_id() const;

    int read();

    void set_on_pos(std::function<void(const Net_pos&)> func);

    void generate_session_code();

    void keepalive();

    void disconnect(uint32_t client_id);

    bool is_full() const;

    bool is_empty() const;

    void broadcast_udp(void* data, uint32_t len);
    void broadcast_tcp(void* data, uint32_t len);

    Net_client* find_client(Net_client* client) const;

    bool is_old(std::chrono::time_point<std::chrono::high_resolution_clock>& now) const;

    char session_code[16];
    mmh::Hash_key session_code_hash;

    void set_num_players(uint8_t num_players);

    uint8_t get_num_players() const;
private:
    void handle_inc_pos(const std::vector<uint8_t>& data);

    void on_udp_data(const std::vector<uint8_t>& data, int32_t data_len);

    std::vector<Net_session_player> _players;

    uint32_t _id;
    uint8_t _max_players;
    uint8_t _num_players;

    uint32_t _keepalive_seconds;

    Tcp_server* _tcp_server;
    Udp_server* _udp_server;

    World_instance _world;

    Net_client* _owner;

    std::function<void(const Net_pos&)> _on_pos;

    std::chrono::time_point<std::chrono::high_resolution_clock> _last_keepalive;
};