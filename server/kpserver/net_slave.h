#pragma once

#include <memory>
#include <vector>
#include <stdint.h>
#include <chrono>

#include "tcp_client.h"
#include "net_client.h"
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
private:
    void on_inc_data(std::shared_ptr<Net_client> client, const std::vector<uint8_t>& data);
    void handle_master_command(const Net_master_to_slave_command& command);
    void print_stats();

    Tcp_server*         _tcp;
    Ini_file            _ini_file;
    Net_master_info     _master;

    std::shared_ptr<Process_stats> _stats;

    std::shared_ptr<Tcp_client> _master_connection;

    std::vector<uint8_t> _data_buffer;

    std::chrono::steady_clock::time_point _next_slave_report;
};