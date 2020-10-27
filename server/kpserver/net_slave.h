#pragma once

#include <memory>
#include <vector>
#include <stdint.h>
#include <chrono>
#include <unordered_map>

#include "udp_server.h"
#include "tcp_client.h"
#include "net_client.h"
#include "net_session.h"
#include "net_session_player.h"
#include "ini_file.h"
#include "net_packet.h"
#include "process_stats.h"

struct Tcp_server;

struct Net_master_info {
    std::shared_ptr<Ini_node> node;
};

struct Net_slave {
    Net_slave(Tcp_server* tcp, const Ini_file& file, std::shared_ptr<Process_stats> stats);
    virtual ~Net_slave();

    bool init();
    void update();
    void setup_sessions();
private:
    void on_inc_client_udp_data(std::shared_ptr<Net_client> client, const std::vector<uint8_t>& data, int32_t data_len);
    void on_inc_client_tcp_data(std::shared_ptr<Net_client> client, const std::vector<uint8_t>& data, int32_t data_len);
    void on_client_connect(std::shared_ptr<Net_client> client);
    void on_client_disconnect(std::shared_ptr<Net_client> client);
    void handle_master_command(const Net_master_to_slave_command& command);
    void print_stats();
    bool connect_to_master();

    Tcp_server*         _tcp;
    
    std::shared_ptr<Udp_server> _udp;
    
    Ini_file            _ini_file;
    Net_master_info     _master;
    std::shared_ptr<Ini_node> _my_node;

    std::shared_ptr<Process_stats> _stats;

    std::shared_ptr<Tcp_client> _master_connection;

    std::vector<uint8_t> _data_buffer;

    std::chrono::time_point<std::chrono::high_resolution_clock> _next_slave_report;


    std::unordered_map<uint32_t, std::shared_ptr<Net_session>>        _session_code_lookup; // this is a hash as a key
    std::unordered_map<uint32_t, std::shared_ptr<Net_session>>        _session_id_lookup;
    std::unordered_map<uint32_t, std::shared_ptr<Net_session_player>> _client_id_lookup;

    std::vector<std::shared_ptr<Net_session>> _sessions;
};