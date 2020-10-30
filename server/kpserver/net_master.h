#pragma once

#include <vector>
#include <stdint.h>
#include <memory>
#include <unordered_map>
#include <chrono>

#include "tcp_server.h"
#include "net_packet.h"
#include "net_client.h"
#include "net_session_player.h"
#include "net_session.h"
#include "hash.h"

struct Tcp_server;
struct Ini_file;
struct Ini_node;

/// <summary>
/// We keep a lightweight record of all sessions on the slaves
/// so we can lookup session codes
/// </summary>
struct Net_session_info {
    uint32_t    id;

    uint16_t    num_players;

    char        code[16];
    
    mmh::Hash_key session_code_hash;

    void set_session_hash() {
        code[8] = '\0';

        session_code_hash = mmh::Hash_key(code);
    }
};

/// <summary>
/// We need to keep track of the connected slaves because:
/// + Players must be able to find private games, so we store all session codes
/// + Players needs to be distributed evenly across all slaves, so we only
///   send out connection info to players on the "best"/least populates slave
/// </summary>
struct Net_slave_info {
    uint32_t        slave_id;

    int32_t         health_rating; // negative is bad health
    
    Net_client*     client;
    
    uint16_t        num_players;
    uint16_t        num_sessions;

    uint32_t        session_id_start_range;

    char            ip[64];
    
    uint16_t        tcp_port;
    uint16_t        udp_port;

    Net_slave_info(Net_client* client_, uint32_t slave_id_, uint32_t session_id_start_range_, uint32_t num_sessions_);

    void set_health_rating(const Net_slave_health_snapshot& snap);

    Net_session_info* get_session(uint32_t session_id);

    //void keepalive();

    void add_session(uint32_t id, const char* code);
    void set_session(uint32_t id, const char* code, uint32_t num_players);
   
    //void clear_old_sessions();

private:
    std::unordered_map<uint32_t, Net_session_info*>     _session_id_lookup;
    std::unordered_map<uint32_t, Net_session_info*>     _session_code_lookup;
    std::vector<std::unique_ptr<Net_session_info>>      _sessions;
    //std::chrono::time_point<std::chrono::high_resolution_clock> _last_keepalive;
};

struct best_health_is_first
{
    inline bool operator() (const std::unique_ptr<Net_slave_info>& struct1, const std::unique_ptr<Net_slave_info>& struct2)
    {
        return (struct1->health_rating > struct2->health_rating);
    }
};

struct least_players_is_first 
{
    inline bool operator() (const std::unique_ptr<Net_slave_info>& struct1, const std::unique_ptr<Net_slave_info>& struct2)
    {
        return (struct1->num_players > struct2->num_players);
    }
};

struct Net_master {
    Net_master(Ini_file* file);
    virtual ~Net_master();

    Net_slave_info* get_slave(Net_client* client);

    void update();
    
private:
    void add_authenticated_slave(Net_client* client, const Net_authenticate_slave& auth);

    void on_inc_tcp_data(Net_client* client, const std::vector<uint8_t>& data, int32_t data_len);
    void on_client_connect(Net_client* client);
    void on_client_disconnect(Net_client* client);

    void remove_slave(Net_client* client);
    void remove_player(Net_client* client);

    std::unordered_map<uint32_t, Net_slave_info*>                       _session_code_lookup; // this is a hash as a key
    std::unordered_map<uint32_t, Net_slave_info*>                       _session_id_lookup;
    
    std::unordered_map<uint32_t, Net_session_player*>                   _client_id_lookup;

    std::unordered_map<uint32_t, Net_slave_info*>                       _slave_by_slave_id; // slave_id is key
    std::unordered_map<uint32_t, Net_slave_info*>                       _slave_by_client_id; // client_id is key
    std::vector<std::unique_ptr<Net_slave_info>>                        _slaves_health;

    std::chrono::time_point<std::chrono::high_resolution_clock>         _next_slave_report;

    std::vector<uint8_t>    _data_buffer;
    Tcp_server              _tcp;

    Ini_file*               _ini_file;
    Ini_node*               _my_node;
};