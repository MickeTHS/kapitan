#pragma once

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>
#include <stdint.h>
#include <memory>

struct Ini_node {
    std::string name;
    std::string hostname;
    
    uint32_t id;
    uint32_t max_users_per_group;
    uint32_t max_groups;
    uint16_t tcp_port;
    uint16_t udp_port;
    uint32_t keepalive_time_seconds;
    uint32_t ticks_per_second_internal; // how often we simulate
    uint32_t ticks_per_second_position_update_sends; // how often we send position/player updates
    uint32_t slave_sync_interval_seconds;
    uint64_t master_password;
    uint64_t client_password;

    bool is_me;
    bool is_master;

    Ini_node();

    void write(std::ofstream& outfile);

    void print() const;
};

struct Ini_file {
    Ini_file() {}

    std::vector<std::shared_ptr<Ini_node>> nodes;

    Ini_file(const std::string& filename);

    std::shared_ptr<Ini_node> get_master() const;
    std::shared_ptr<Ini_node> get_me() const;
    void get_slaves(std::vector<std::shared_ptr<Ini_node>>& slaves) const;

    void print_help();
    bool save();
    bool read();
    
    std::string _filepath;

private:
    
};
