#pragma once

#include <vector>
#include <stdint.h>
#include <memory>
#include <unordered_map>
#include <chrono>

#include "net_packet.h"
#include "net_client.h"
#include "net_session_player.h"
#include "net_session.h"
#include "hash.h"
#include "ini_file.h"

struct Tcp_server;

struct Net_slave_info {
    Net_slave_info(Net_client* client_, uint32_t slave_id_);

    int32_t health_rating; // negative is bad health
    uint32_t slave_id;
    Net_client* client;

    void set_health_rating(const Net_slave_health_snapshot& snap) {
        // calculate a health rating used to determine the healthiest node
        health_rating = snap.avg_tick_idle_time * ((1.0f - snap.pct_good_vs_lag_ticks) * 100);
    }
};

struct best_health_is_first
{
    inline bool operator() (const std::shared_ptr<Net_slave_info>& struct1, const std::shared_ptr<Net_slave_info>& struct2)
    {
        return (struct1->health_rating > struct2->health_rating);
    }
};

struct Net_master {
    Net_master(Tcp_server* tcp, const Ini_file& file);
    virtual ~Net_master();

    std::shared_ptr<Net_slave_info> get_slave(Net_client* client);

    void update();
    
private:
    void add_authenticated_slave(Net_client* client, const Net_authenticate_slave& auth);

    void on_inc_tcp_data(Net_client* client, const std::vector<uint8_t>& data, int32_t data_len);
    void on_client_connect(Net_client* client);
    void on_client_disconnect(Net_client* client);

    void remove_slave(Net_client* client);
    void remove_player(Net_client* client);

    std::unordered_map<uint32_t, std::shared_ptr<Net_session>>      _session_code_lookup; // this is a hash as a key
    std::unordered_map<uint32_t, std::shared_ptr<Net_session>>      _session_id_lookup;
    std::unordered_map<uint32_t, std::shared_ptr<Net_session_player>> _client_id_lookup;

    Tcp_server* _tcp;

    Ini_file _ini_file;
    std::shared_ptr<Ini_node> _my_node;

    std::unordered_map<uint32_t, std::shared_ptr<Net_slave_info>> _slave_by_slave_id; // slave_id is key
    std::unordered_map<uint32_t, std::shared_ptr<Net_slave_info>> _slave_by_client_id; // client_id is key
    std::vector<std::shared_ptr<Net_slave_info>> _slaves_health;

    std::chrono::time_point<std::chrono::high_resolution_clock> _next_slave_report;

    std::vector<uint8_t> _data_buffer;
};