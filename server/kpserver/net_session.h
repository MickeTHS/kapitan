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
#include "world_snapshot.h"

struct Tcp_server;
struct Udp_server;
struct Net_client_info;
struct Net_client;

enum GameRule {
    PlayerMovementSpeed = 0,
    GameSeconds,
    MaxRules
};

/// a group is a collection of clients that will be playing a private game
/// that once it has been started, only these clients will be able to 
/// communicate among each other
struct Net_session {
    static uint32_t __SESSION_ID_COUNTER;

    Net_session(uint8_t max_players, uint32_t keepalive_seconds, Tcp_server* tcp_server, Udp_server* udp_server);

    virtual ~Net_session();

    bool add_player_and_broadcast(Net_client* info, bool broadcast, bool is_owner);

    int32_t on_tcp_data(const std::vector<uint8_t>& data, uint32_t offset, int32_t len);

    uint32_t get_id() const;

    int read();

    void on_inc_pos(const Net_pos& pos);

    void set_on_pos(std::function<void(const Net_pos&)> func);

    void generate_session_code();

    void set_private(bool priv);

    bool is_private() const;

    void keepalive();

    void disconnect(uint32_t client_id);

    bool is_full() const;

    bool is_empty() const;

    void broadcast_udp(void* data, uint32_t len);
    void broadcast_tcp(void* data, uint32_t len);

    Net_client* find_client(Net_client* client) const;

    bool is_old(std::chrono::time_point<std::chrono::high_resolution_clock>& now) const;

    char session_code[8];
    mmh::Hash_key session_code_hash;

    void set_num_players(uint8_t num_players);

    uint8_t get_num_players() const;

    uint8_t get_max_players() const;

    void remove_player(Net_client* client);

    bool remove_player_and_broadcast(Net_client* client, bool broadcast);

    uint32_t get_time() const;

    uint64_t get_end_time() const;

    uint64_t get_start_time() const;

    bool can_game_be_started() const;

    void start_game_in_seconds(int seconds);

    void end_game();

    void update_game(const double delta, std::chrono::time_point<std::chrono::high_resolution_clock>& now); 

    void send_udp(std::chrono::time_point<std::chrono::high_resolution_clock>& now);
    
    bool set_game_rule(uint16_t rule_id, uint16_t rule_value);

    void msg_item_set(Net_client* client, const Net_player_set_item_state_request& request);

    Net_game_config game_config;
private:

    void on_time_for_snapshot();

    void on_game_started(std::chrono::time_point<std::chrono::high_resolution_clock>& now);

    void on_game_ended(std::chrono::time_point<std::chrono::high_resolution_clock>& now);

    void handle_inc_pos(const std::vector<uint8_t>& data);

    void on_udp_data(const std::vector<uint8_t>& data, int32_t data_len);

    std::vector<Net_session_player> _players;

    uint32_t _id;
    uint8_t _max_players;
    uint8_t _num_players;

    bool _is_private;

    uint32_t _keepalive_seconds;
    uint32_t _session_timestamp;

    double _time_since_snapshot;
    
    Tcp_server* _tcp_server;
    Udp_server* _udp_server;

    Net_client* _owner;

    std::function<void(const Net_pos&)> _on_pos;

    std::chrono::time_point<std::chrono::high_resolution_clock> _last_keepalive;

    std::chrono::time_point<std::chrono::high_resolution_clock> _now;

    std::chrono::time_point<std::chrono::high_resolution_clock> _last_tick;

    std::chrono::time_point<std::chrono::high_resolution_clock> _game_start_time;

    std::chrono::time_point<std::chrono::high_resolution_clock> _game_end_time;

    std::unique_ptr<World_instance> _world;

    

    bool _game_running;
    bool _game_starting;

    std::vector<uint8_t> _data_buffer;

    Net_session_snapshot _snapshot;
};