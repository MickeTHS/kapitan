#pragma once

#include <stdint.h>
#include <memory>
#include <vector>
#include <stdio.h>

/// The packet we send to clients as well as receive

enum class MsgType {
    GroupConfig = 0,
    PlayerPos = 1,
    OtherPos = 2
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
    uint8_t type;
    uint32_t group_id;
    uint16_t pos_port;
    char hostname[64];

    Net_group_config() {
        type = (uint8_t)MsgType::GroupConfig;
        group_id = 0;
        pos_port = 0;
        memset(hostname, 0, 64);
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t offset) {
        memcpy(&data[offset], &type, 1);
        memcpy(&data[offset + sizeof(uint8_t)], &group_id, sizeof(group_id));
        memcpy(&data[offset + sizeof(uint8_t) + sizeof(group_id)], &pos_port, sizeof(pos_port));
        memcpy(&data[offset + sizeof(uint8_t) + sizeof(group_id) + sizeof(pos_port)], hostname, 64);
    }
};

struct Net_pos {
    uint8_t type;
    uint16_t entity;
    std::vector<int16_t> pos;

    Net_pos() {
        type = (uint8_t)MsgType::PlayerPos;
        entity = 0;
    }

    void from_buffer(const std::vector<uint8_t>& data) {
        pos.resize(3);

        memcpy(&type, &data[0], sizeof(type));
        memcpy(&entity, &data[sizeof(type)], sizeof(entity));
        memcpy(&pos[0], &data[sizeof(type) + sizeof(entity)], sizeof(int16_t) * 3);
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t offset) {
        memcpy(&data[offset], &type, sizeof(type));
        memcpy(&data[offset + sizeof(type)], &entity, sizeof(entity));
        memcpy(&data[offset + sizeof(type) + sizeof(entity)], &pos[0], sizeof(int16_t) * 3);
    }

    void print() {
        printf("[MSG-POS][E: %d][x: %d][y: %d][z: %d]\n", entity, pos[0], pos[1], pos[2]);
        printf("[MSG-POS][DEBUG][x: %f][y: %f][z: %f]\n", ((float)pos[0]) / 100.0f, ((float)pos[1]) / 100.0f, ((float)pos[2]) / 100.0f);
    }

};

struct Net_packet {
    Net_packet(uint32_t size);
    virtual ~Net_packet();

    std::vector<uint8_t> data;
};