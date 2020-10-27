#include "ini_file.h"
#include <string>
#include <vector>

Ini_node::Ini_node() {
    hostname = "localhost";
    ip = "";
    name = "test";
    id = 0;
    max_users_per_group = 6;
    max_groups = 50;
    is_master = false;
    tcp_port = 8888;
    udp_port = 9000;
    keepalive_time_seconds = 60;
    ticks_per_second_internal = 20;
    ticks_per_second_position_update_sends = 20;
    slave_sync_interval_seconds = 60;
    master_password = 0;
    client_password = 0;
    is_me = false;
    is_ip_set = false;
}

void Ini_node::print() const {
    if (is_master) {
        printf("[MASTER]\n");
    }
    else {
        printf("[SLAVE]\n");
    }

    printf("name: %s, hostname: %s, id: %d, master: %s, tcp_port: %d, udp_port: %d\n", 
        name.c_str(),
        hostname.c_str(),
        id,
        is_master ? "Yes" : "No",
        tcp_port,
        udp_port
    );
}

void Ini_node::write(std::ofstream& outfile) {
    if (is_master) {
        outfile << "[MASTER]" << std::endl;
    }
    else {
        outfile << "[SLAVE]" << std::endl;
    }

    outfile << "name:" << name << std::endl;
    outfile << "hostname:" << hostname << std::endl;
    outfile << "ip:" << ip << std::endl;
    outfile << "tcp_port:" << tcp_port << std::endl;
    outfile << "udp_port:" << udp_port << std::endl;
    outfile << "id:" << id << std::endl;
    outfile << "max_users_per_group:" << max_users_per_group << std::endl;
    outfile << "max_groups:" << max_groups << std::endl;
    outfile << "keepalive_time_seconds:" << keepalive_time_seconds << std::endl;
    outfile << "ticks_per_second_internal:" << ticks_per_second_internal << std::endl;
    outfile << "ticks_per_second_position_update_sends:" << ticks_per_second_position_update_sends << std::endl;
    outfile << "slave_sync_interval_seconds:" << slave_sync_interval_seconds << std::endl;
    outfile << "master_password:" << master_password << std::endl;
    outfile << "is_me:" << (is_me ? "yes" : "no") << std::endl;
    outfile << "client_password:" << client_password << std::endl;
    outfile << "" << std::endl;
}

Ini_file::Ini_file (const std::string& filename) {
    _filepath = filename;
}

std::shared_ptr<Ini_node> Ini_file::get_master() const {
    for (auto node : nodes) {
        if (node->is_master) {
            return node;
        }
    }

    return nullptr;
}

std::shared_ptr<Ini_node> Ini_file::get_me() const {
    for (auto node : nodes) {
        if (node->is_me) {
            return node;
        }
    }

    return nullptr;
}

void Ini_file::get_slaves(std::vector<std::shared_ptr<Ini_node>>& slaves) const {
    auto me = get_me();

    for (auto node : nodes) {
        if (node != me) {
            slaves.push_back(node);
        }
    }
}

void Ini_file::print_help() {
    printf("Ini usage: \n");
}

bool Ini_file::save() {
    std::ofstream outfile;
    outfile.open(_filepath);


    for (auto n : nodes) {
        n->write(outfile);
    }

    outfile.close();

    return true;
}

bool Ini_file::read() {
    std::ifstream infile(_filepath);
    std::string line;

    if (!infile.good()) {
        printf("ERROR: ini file: %s, not found\n", _filepath.c_str());
        return false;
    }

    std::vector<std::string> keys;

    std::shared_ptr<Ini_node> current_node = nullptr;

    while(getline(infile, line)) {
        if (line.length() <= 0 || line[0] == '#')
            continue;
        
        if (line.find("[MASTER]") != std::string::npos) {
            keys.clear();
            current_node = std::make_shared<Ini_node>();
            nodes.push_back(current_node);
            
            current_node->is_master = true;
        }
        else if (line.find("[SLAVE]") != std::string::npos) {
            keys.clear();
            current_node = std::make_shared<Ini_node>();
            
            nodes.push_back(current_node);

            current_node->is_master = false;
        }

        std::istringstream is_line(line);
        
        std::string key;
        std::string value;
        
        getline(is_line, key, ':');
        getline(is_line, value);
        
        if (key == "hostname") {
            current_node->hostname = value;
        }
        else if (key == "ip") {
            current_node->ip = value;
            current_node->is_ip_set = true;
        }
        else if (key == "name") {
            current_node->name = value;
        }
        else if (key == "id") {
            current_node->id = stoi(value);
        }
        else if (key == "tcp_port") {
            current_node->tcp_port = stoi(value);
        }
        else if (key == "udp_port") {
            current_node->udp_port = stoi(value);
        }
        else if (key == "max_users_per_group") {
            current_node->max_users_per_group = stoi(value);
        }
        else if (key == "max_groups") {
            current_node->max_groups = stoi(value);
        }
        else if (key == "keepalive_time_seconds") {
            current_node->keepalive_time_seconds = stoi(value);
        }
        else if (key == "master_password") {
            current_node->master_password = std::stoull(value, nullptr, 10);
        }
        else if (key == "ticks_per_second_internal") {
            current_node->ticks_per_second_internal = stoi(value);
        }
        else if (key == "ticks_per_second_position_update_sends") {
            current_node->ticks_per_second_position_update_sends = stoi(value);
        }
        else if (key == "slave_sync_interval_seconds") {
            current_node->slave_sync_interval_seconds = stoi(value);
        }
        else if (key == "is_me") {
            current_node->is_me = value == "yes";
        }
        else if (key == "client_password") {
            current_node->client_password = std::stoull(value, nullptr, 10);
        }
    }

    for (auto n : nodes) {
        n->print();
    }

    return true;
}

