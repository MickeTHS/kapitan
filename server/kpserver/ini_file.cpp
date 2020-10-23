#include "ini_file.h"
#include <string>
#include <vector>

Ini_node::Ini_node() {
    hostname = "localhost";
    name = "test";
    id = 0;
    max_users_per_group = 6;
    max_groups = 50;
    port = 8888;
    is_main = false;
    is_master = false;
    udp_range_max = 2900;
    udp_range_min = 2500;
}

void Ini_node::print() const {
    if (is_main && is_master) {
        printf("[MASTER]\n");
    }
    else {
        printf("[SLAVE]\n");
    }

    printf("name: %s, hostname: %s, id: %d, port: %d, is_main: %s, is_master: %s, udp_range_max: %d, udp_range_min: %d\n", 
        name.c_str(),
        hostname.c_str(),
        id,
        port,
        is_main ? "Yes" : "No",
        is_master ? "Yes" : "No",
        udp_range_max,
        udp_range_min
    );
}

void Ini_node::write(std::ofstream& outfile) {
    if (is_main && is_master) {
        outfile << "[MASTER]" << std::endl;
    }
    else {
        outfile << "[SLAVE]" << std::endl;
    }

    outfile << "name:" << name << std::endl;
    outfile << "hostname:" << hostname << std::endl;
    outfile << "port:" << port << std::endl;
    outfile << "id:" << id << std::endl;
    outfile << "max_users_per_group:" << max_users_per_group << std::endl;
    outfile << "max_groups:" << max_groups << std::endl;
    outfile << "udp_range_min:" << udp_range_min << std::endl;
    outfile << "udp_range_max:" << udp_range_max << std::endl;
    outfile << "" << std::endl;
}

Ini_file::Ini_file (const std::string& filename) {
    _filepath = filename;
}

void Ini_file::print_help() {
    printf("Ini usage: \n");
}

bool Ini_file::save() {
    std::ofstream outfile;
    outfile.open(_filepath);

    if (node == nullptr) {
        printf("ERROR: impossible to save ini file\n");
        return false;
    }

    node->write(outfile);

    for (auto n : children) {
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
            node = current_node;
            node->is_master = true;
            node->is_main = true;
        }
        else if (line.find("[SLAVE]") != std::string::npos) {
            keys.clear();
            current_node = std::make_shared<Ini_node>();
            
            if (node != nullptr && node->is_main) {
                children.push_back(current_node);
                current_node->is_main = false;
                current_node->is_master = false;
            }
            else {
                node = current_node;
                node->is_main = true;
                node->is_master = false;
            }
        }

        std::istringstream is_line(line);
        
        std::string key;
        std::string value;
        
        getline(is_line, key, ':');
        getline(is_line, value);
        
        if (key == "hostname") {
            current_node->hostname = value;
        }
        else if (key == "name") {
            current_node->name = value;
        }
        else if (key == "id") {
            current_node->id = stoi(value);
        }
        else if (key == "port") {
            current_node->port = stoi(value);
        }
        else if (key == "max_users_per_group") {
            current_node->max_users_per_group = stoi(value);
        }
        else if (key == "max_groups") {
            current_node->max_groups = stoi(value);
        }
    }

    node->print();

    for (auto n : children) {
        n->print();
    }

    return true;
}

