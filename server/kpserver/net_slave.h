#pragma once

#include <memory>
#include <vector>
#include <stdint.h>
#include <chrono>
#include <unordered_map>

#include "udp_server.h"
#include "tcp_server.h"
#include "tcp_client.h"
#include "net_client.h"
#include "net_session.h"
#include "net_session_player.h"
#include "net_packet.h"
#include "process_stats.h"

struct Ini_file;
struct Ini_node;

struct Net_master_info {
    Ini_node* node;
};

struct Net_slave {
    Net_slave(Ini_file* file, Process_stats* stats);
    virtual ~Net_slave();

    bool init();
    void update();
    void setup_sessions();
    void print_sessions() const;
    void print_sessions_summary() const;
    bool validate_ini();

    void set_session_private(Net_session* session, bool priv);
private:
    void on_inc_tcp_master_data(const std::vector<uint8_t>& data, int32_t data_len);
    void on_inc_client_udp_data(Net_client* client, const std::vector<uint8_t>& data, int32_t data_len);
    void on_inc_client_tcp_data(Net_client* client, const std::vector<uint8_t>& data, int32_t data_len);
    void on_client_connect(Net_client* client);
    void on_client_disconnect(Net_client* client);
    void handle_master_command(const Net_master_to_slave_command& command);
    void print_stats();
    bool connect_to_master();
    bool find_client(Net_client* client) const;
    Net_session* find_client_session(Net_client* client) const;
    Net_session* find_empty_session() const;

    void sync_session_with_master(Net_session* session);

    void reset_session_lookups(Net_session* session);

    void set_session_lookups(Net_session* session);
    
    Tcp_server                                  _tcp;
    Udp_server                                  _udp;
    
    Ini_file*                                   _ini_file;
    Net_master_info                             _master;
    Ini_node*                                   _my_node;

    Process_stats*                              _stats;

    Tcp_client                                  _master_connection;

    std::vector<uint8_t>                        _data_buffer;

    std::chrono::time_point<std::chrono::high_resolution_clock> _next_slave_report;

    std::chrono::time_point<std::chrono::high_resolution_clock> _last_tick;

    std::unordered_map<uint32_t, Net_session*>  _session_code_lookup; // this is a hash as a key
    std::unordered_map<uint32_t, Net_session*>  _session_id_lookup;
    std::unordered_map<uint32_t, std::shared_ptr<Net_session_player>> _client_id_lookup;

    std::vector<std::unique_ptr<Net_session>>   _sessions;

    std::vector<Net_session*>                   _private_sessions;
    std::vector<Net_session*>                   _public_sessions;

    uint32_t                                    _num_players;

    uint32_t                                    _tick_check;
};