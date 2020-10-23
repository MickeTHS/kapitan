#pragma once

#include <vector>
#include <stdint.h>
#include <memory>

#include "net_client.h"

struct Con_socket;

struct Net_master {
    Net_master(Con_socket* tcp);
    virtual ~Net_master();

private:
    void on_inc_data(std::shared_ptr<Net_client> client, const std::vector<uint8_t>& data);

    Con_socket* _tcp;
};