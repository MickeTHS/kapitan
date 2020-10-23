#pragma once

#include <stdint.h>
#include <memory>
#include <vector>
#include <stdio.h>

/// The packet we send to clients as well as receive

enum class MsgType {
    GroupConfig = 0,
    PlayerPos = 1,
    OtherPos = 2,
    NetPlayerRequestSlaveNode,
    NetSlaveConfig,
    NetSlaveSessionSummary,
    NetPlayer,
    NetPlayerRequestSlaveConfig,
    NetSnapshotPlayer
};

/// <summary>
/// this is the struct we use for the snapshot
/// which is a lightweight struct that tells
/// all relevant information about the player
/// </summary>
struct Net_snapshot_player {
    uint8_t type;
    uint16_t entity_id;
    uint8_t hp;
    uint8_t prone_status; // 0 -> dead, 1 -> alive, 2 -> unconscious
    uint8_t engagement_status; // 0 -> idle, 1 -> hacking, 2 -> grinding door
};

struct Net_player {
    uint8_t type;
    uint16_t entity_id;
    char username[64];
    uint16_t avatar[32];

    Net_player(const std::vector<uint8_t>& data) {
        memcpy(this, &data[0], sizeof(Net_player));
    }

    Net_player() {
        type = (uint8_t)MsgType::NetPlayer;
        entity_id = 0;
        memset(avatar, 0, sizeof(uint16_t) * 32);
        memset(username, 0, 64);
    }

    void from_buffer(const std::vector<uint8_t>& data, uint32_t offset) {
        memcpy(&type, &data[offset], sizeof(type));
        memcpy(&entity_id, &data[offset + sizeof(type)], sizeof(entity_id));
        memcpy(&username[0], &data[offset + sizeof(type) + sizeof(entity_id)], 64);
        memcpy(&avatar[0], &data[offset + sizeof(type) + sizeof(entity_id) + 64], 32 * sizeof(uint16_t));
    }

    bool set_buffer(std::vector<uint8_t>& data, uint32_t offset) {
        memcpy(&data[offset], &type, sizeof(type));
        memcpy(&data[offset + sizeof(type)], &entity_id, sizeof(entity_id));
        memcpy(&data[offset + sizeof(type) + sizeof(entity_id)], username, 64);
        memcpy(&data[(uint64_t)offset + sizeof(type) + sizeof(entity_id) + 64], avatar, sizeof(uint16_t) * 32);

        return true;
    }

    void set_buffer_fast(std::vector<uint8_t>&data, uint32_t offset) {
        memcpy(&data[offset], this, sizeof(Net_player));
    }
};

/// <summary>
/// Sent to a master node to get a "good" slave node
/// Should send back a Net_slave_config to the player
/// </summary>
struct Net_player_request_slave_node {
    uint8_t type;

    Net_player_request_slave_node() {
        type = (uint8_t)MsgType::NetPlayerRequestSlaveNode;
    }
};

struct Net_player_request_slave_config {
    uint8_t type;

    Net_player_request_slave_config() {
        type = (uint8_t)MsgType::NetPlayerRequestSlaveConfig;
    }
};

/// <summary>
/// Gets a summary configuration to the slave node so players can connect to it
/// </summary>
struct Net_slave_config {
    uint8_t type;
    uint32_t node_id;
    uint32_t port;

    char hostname[64];
    
    Net_slave_config(const std::vector<uint8_t>& data) {
        memcpy(this, &data[0], sizeof(Net_slave_config));
    }

    Net_slave_config() {
        type = (uint8_t)MsgType::NetSlaveConfig;
        port = 0;
        node_id = 0;
        memset(hostname, 0, 64);
    }

    void from_buffer(const std::vector<uint8_t>& data) {
        memcpy(&type, &data[0], sizeof(type));
        memcpy(&node_id, &data[sizeof(type)], sizeof(node_id));
        memcpy(&port, &data[sizeof(type) + sizeof(node_id)], sizeof(port));
        memcpy(&hostname[0], &data[sizeof(type) + sizeof(node_id) + sizeof(port)], 64);
    }

    void set_buffer_fast(std::vector<uint8_t>& data, uint32_t offset) {
        memcpy(&data[offset], this, sizeof(Net_slave_config));
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t offset) {
        memcpy(&data[offset], &type, sizeof(type));
        memcpy(&data[offset + sizeof(type)], &node_id, sizeof(node_id));
        memcpy(&data[offset + sizeof(type) + sizeof(node_id)], &port, sizeof(port));
        memcpy(&data[offset + sizeof(type) + sizeof(node_id) + sizeof(port)], &hostname, 64);
    }
};

/// <summary>
/// Lists all players in a session
/// </summary>
struct Net_slave_session_players_list {
    uint8_t type;
    uint8_t num_players;

    std::vector<Net_player> players;

    void from_buffer(const std::vector<uint8_t>& data, uint32_t offset) {
        memcpy(&type, &data[0], sizeof(type));
        memcpy(&num_players, &data[sizeof(type)], sizeof(num_players));

        players.resize(num_players);

        for (int i = 0; i < num_players; ++i) {
            players[i].from_buffer(data, sizeof(type) + sizeof(num_players) + sizeof(Net_player) * i);
        }
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t offset) {
        memcpy(&data[offset], &type, sizeof(type));
        memcpy(&data[offset + sizeof(type)], &num_players, sizeof(num_players));

        for (int i = 0; i < num_players; ++i) {
            players[i].set_buffer(data, offset + sizeof(type) + sizeof(num_players) + sizeof(Net_player) * i);
        }
    }

    void set_buffer_fast(std::vector<uint8_t>& data, uint32_t offset) {
        memcpy(&data[offset], this, sizeof(uint8_t) + sizeof(uint8_t));

        for (int i = 0; i < num_players; ++i) {
            //memcpy(&data[offset + sizeof(uint8_t) + sizeof(uint8_t) + sizeof(Net_player) * i], &players[i], sizeof(Net_player));
            players[i].set_buffer_fast(data, offset + sizeof(uint8_t) + sizeof(uint8_t) + sizeof(Net_player) * i);
        }
    }
};


/// <summary>
/// A summary of a session
/// </summary>
struct Net_slave_session_summary {
    uint8_t type;
    uint32_t session_id;
    uint8_t max_players;
    uint8_t num_players;

    void from_buffer(const std::vector<uint8_t>& data, uint32_t offset) {
        memcpy(&type, &data[offset], sizeof(type));
        memcpy(&session_id, &data[offset + sizeof(type)], sizeof(session_id));
        memcpy(&max_players, &data[offset + sizeof(type) + sizeof(session_id)], sizeof(max_players));
        memcpy(&num_players, &data[offset + sizeof(type) + sizeof(session_id) + sizeof(max_players)], sizeof(num_players));
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t offset) {
        memcpy(&data[offset], &type, sizeof(type));
        memcpy(&data[offset + sizeof(type)], &session_id, sizeof(session_id));
        memcpy(&data[offset + sizeof(type) + sizeof(session_id)], &max_players, sizeof(max_players));
        memcpy(&data[offset + sizeof(type) + sizeof(session_id) + sizeof(max_players)], &num_players, sizeof(num_players));
    }
};

/// <summary>
/// Lists all sessions in the node
/// </summary>
struct Net_slave_session_list {
    uint8_t type;
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
    std::vector<int16_t> rot;

    Net_pos() {
        type = (uint8_t)MsgType::PlayerPos;
        entity = 0;
    }

    void from_buffer(const std::vector<uint8_t>& data) {
        pos.resize(3);
        rot.resize(4);

        memcpy(&type, &data[0], sizeof(type));
        memcpy(&entity, &data[sizeof(type)], sizeof(entity));
        memcpy(&pos[0], &data[sizeof(type) + sizeof(entity)], sizeof(int16_t) * 3);
        memcpy(&rot[0], &data[sizeof(type) + sizeof(entity) + sizeof(int16_t) * 3], sizeof(int16_t) * 4);
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t offset) {
        memcpy(&data[offset], &type, sizeof(type));
        memcpy(&data[offset + sizeof(type)], &entity, sizeof(entity));
        memcpy(&data[offset + sizeof(type) + sizeof(entity)], &pos[0], sizeof(int16_t) * 3);
        memcpy(&data[offset + sizeof(type) + sizeof(entity) + sizeof(int16_t) * 3], &rot[0], sizeof(int16_t) * 4);
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