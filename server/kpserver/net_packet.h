#pragma once

#include <stdint.h>
#include <memory>
#include <vector>
#include <stdio.h>

/// The packet we send to clients as well as receive

enum class MsgType {
    GroupConfig = 0
};

struct Net_player {
    char username[64];
    uint16_t avatar[32];

    bool set_buffer(std::vector<uint8_t>& data, uint32_t offset) {
        if (offset + sizeof(Net_player) > data.size()) {
            printf("ERROR: Net_player buffer size exceeded\n");
            return false;
        }

        memcpy(&data[offset], username, 64);
        memcpy(&data[(uint64_t)offset + 64], avatar, sizeof(uint16_t) * 32);

        return true;
    }
};

struct Net_group_config {
    MsgType type;
    uint32_t group_id;
    uint16_t pos_port;
    char hostname[64];

    Net_group_config() {
        type = MsgType::GroupConfig;
        group_id = 0;
        pos_port = 0;
        memset(hostname, 0, 64);
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t offset) {
        memcpy(&data[offset], &type, sizeof(MsgType));
        memcpy(&data[offset + sizeof(MsgType)], &group_id, sizeof(group_id));
        memcpy(&data[offset + sizeof(MsgType) + sizeof(group_id)], &pos_port, sizeof(pos_port));
        memcpy(&data[offset + sizeof(MsgType) + sizeof(group_id) + sizeof(pos_port)], hostname, 64);
    }
};

struct Net_pos {
    Net_pos(const std::vector<uint8_t>& data) {
        pos.resize(3);

        memcpy(&pos[0], &data[0], sizeof(float) * 3);

        printf("Pos: %f, %f, %f\n", pos[0], pos[1], pos[2]);
    }

    //uint16_t target;
    std::vector<float> pos;
};

struct Net_packet {
    Net_packet(uint32_t size);
    virtual ~Net_packet();

    std::vector<uint8_t> data;
};