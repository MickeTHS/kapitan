#pragma once

#include <vector>
#include <stdint.h>
#include <memory>
#include <unordered_map>
#include <chrono>

#include "net_client.h"
#include "net_session_player.h"
#include "net_session.h"
#include "hash.h"
#include "ini_file.h"

struct Tcp_server;

struct Net_slave_info {
    Net_slave_info(std::shared_ptr<Ini_node> slave_config);

    std::shared_ptr<Ini_node> config;
};

struct Net_master {
    Net_master(Tcp_server* tcp, const Ini_file& file);
    virtual ~Net_master();

    void on_client_connect(std::shared_ptr<Net_client> client);

    void update();
private:
    void on_inc_data(std::shared_ptr<Net_client> client, const std::vector<uint8_t>& data);

    std::unordered_map<uint32_t, std::shared_ptr<Net_session>>      _session_code_lookup; // this is a hash as a key
    std::unordered_map<uint32_t, std::shared_ptr<Net_session>>      _session_id_lookup;

    std::unordered_map<uint32_t, std::shared_ptr<Net_session_player>> _client_id_lookup;

    Tcp_server* _tcp;

    Ini_file _ini_file;
    std::shared_ptr<Ini_node> _my_node;
    std::vector<std::shared_ptr<Net_slave_info>> _slaves;

    std::chrono::steady_clock::time_point _next_slave_report;

    std::vector<uint8_t> _data_buffer;
};