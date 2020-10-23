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
    uint32_t port;
    uint32_t max_users_per_group;
    uint32_t max_groups;
    uint32_t udp_range_min;
    uint32_t udp_range_max;

    bool is_main;
    bool is_master;

    Ini_node();

    void write(std::ofstream& outfile);

    void print() const;
};

struct Ini_file {
    std::shared_ptr<Ini_node> node;

    std::vector<std::shared_ptr<Ini_node>> children;

    Ini_file(const std::string& filename);

    void print_help();
    bool save();
    bool read();
    
    std::string _filepath;

private:
    
};
