#pragma once

#include <vector>
#include <stdint.h>

struct Msg {
    Msg();
    virtual ~Msg();
};

struct Msg_conv {
    Msg_conv();
    virtual ~Msg_conv();

    bool convert(const std::vector<uint8_t>& data);
};