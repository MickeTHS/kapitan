#pragma once

#include <stdint.h>
#include <memory>
#include <vector>
#include <stdio.h>

#pragma pack(push, 1)

/// The packet we send to clients as well as receive

enum class MsgType {
    None = -1,
    GroupConfig = 0,
    PlayerPos = 1,
    OtherPos = 2,
    NetPlayerRequestSlaveNode,
    NetSlaveConfig,
    NetSlaveSessionSummary,
    NetPlayer,
    NetPlayerRequestSlaveConfig,
    NetSnapshotPlayer,
    NetAuthenticatePlayer,
    NetAuthenticateSlave,
    NetFromSlaveToMasterKeepaliveSession,
    NetFromMasterToSlaveCommand,
    NetPlayerJoinSession,
    NetPlayerJoinNotify,
    NetPlayerLeaveSession,
    NetError,
    NetSlaveHealthReport,
    NetPlayerSetGameRuleInt
};

enum class NetErrorType {
    None = 0,
    SessionNotFound,
};


enum class NetMasterToSlaveCommand {
    None = 0,
    ReportHealth
};

//// ########## COMMON PACKETS ################### ////

struct Net_error {
    uint8_t type;
    uint8_t error;

    Net_error(NetErrorType error_type) {
        type = (uint8_t)MsgType::NetError;
        error = (uint8_t)error_type;
    }

};

//// ############# END COMMON PACKETS ############ ////


//// ########### MASTER NODE PACKETS ############# ////

/// <summary>
/// Slaves must send this signal on initiation so they get registered on the master node
/// </summary>
struct Net_authenticate_slave {
    uint8_t type;
    uint32_t slave_id;
    uint64_t master_password;

    Net_authenticate_slave(uint32_t slave_id_, uint64_t master_password_) {
        type = (uint8_t)MsgType::NetAuthenticateSlave;
        slave_id = slave_id_;
        master_password = master_password_;
    }

    Net_authenticate_slave(const std::vector<uint8_t>& data) {
        memcpy(this, &data[0], sizeof(Net_authenticate_slave));
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t offset) {
        memcpy(&data[offset], this, sizeof(Net_authenticate_slave));
    }
};

/// <summary>
/// When a player want to leave a session
/// </summary>
struct Net_player_leave_session {
    uint8_t type;
    uint32_t player_id;
    uint32_t session_id;

    Net_player_leave_session() : type((uint8_t)MsgType::NetPlayerLeaveSession), player_id(0), session_id(0) { }

    Net_player_leave_session(const std::vector<uint8_t>& data, uint32_t off) {
        memcpy(this, &data[off], sizeof(Net_player_leave_session));
    }
};

/// <summary>
/// Notification sent to other players that a player has joined the session
/// </summary>
struct Net_player_join_notify {
    uint8_t type;
    uint32_t player_id;
    char username[64];
    uint16_t avatar[32];

    Net_player_join_notify() : type((uint8_t)MsgType::NetPlayerJoinNotify), player_id(0) {
        memset(username, 0, 64);
        memset(avatar, 0, 32 * sizeof(uint16_t));
    }

    Net_player_join_notify(const std::vector<uint8_t>& data, uint32_t offset) {
        memcpy(this, &data[offset], sizeof(Net_player_join_notify));
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t offset) {
        memcpy(&data[offset], &type, sizeof(Net_player_join_notify));
    }
};

/// <summary>
/// When a player wants to join a session
/// </summary>
struct Net_player_join_session {
    uint8_t type;
    char code[16];

    Net_player_join_session() {
        type = (uint8_t)MsgType::NetPlayerJoinSession;
        memset(code, 0, 16);
    }

    Net_player_join_session(const std::vector<uint8_t>& data, uint32_t off) {
        memcpy(this, &data[off], sizeof(Net_player_join_session));
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

/// <summary>
/// Sends a signal to the slave
/// send command 0 to force a keepalive message from the slave
/// </summary>
struct Net_master_to_slave_command {
    uint8_t type;
    uint8_t command; // 0 -> report keepalive, 1 -> shutdown sessions, 2 -> process shutdown

    Net_master_to_slave_command() {
        type = (uint8_t)MsgType::NetFromMasterToSlaveCommand;
        command = 0;
    }

    Net_master_to_slave_command(NetMasterToSlaveCommand cmd) {
        type = (uint8_t)MsgType::NetFromMasterToSlaveCommand;
        command = (uint8_t)cmd;
    }

    Net_master_to_slave_command(const std::vector<uint8_t>& data, uint32_t offset) {
        memcpy(this, &data[offset], sizeof(Net_master_to_slave_command));
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t offset) {
        memcpy(&data[offset], &type, sizeof(uint8_t));
        memcpy(&data[offset + sizeof(uint8_t)], &command, sizeof(uint8_t));
    }
};

/// <summary>
/// A minimal health report sent from the slave to the master
/// to be used to determine if the slave node is a "good" candidate
/// for more sessions
/// </summary>
struct Net_slave_health_snapshot {
    uint8_t type;

    uint16_t pct_virt_process_ram_used;
    uint16_t pct_good_vs_lag_ticks;
    int64_t avg_tick_idle_time;
    uint16_t pct_cpu_load_process;

    Net_slave_health_snapshot(float virt_process_ram_used, float good_vs_lag_ticks, int64_t avg_idle_time, float cpu_load) {
        type = (uint8_t)MsgType::NetSlaveHealthReport;
        pct_virt_process_ram_used = (uint16_t)(virt_process_ram_used * 10000.0f);
        pct_good_vs_lag_ticks = (uint16_t)(good_vs_lag_ticks * 10000.0f);
        pct_cpu_load_process = (uint16_t)(cpu_load * 10000.0f);

        avg_tick_idle_time = avg_idle_time;
    }

    Net_slave_health_snapshot(const std::vector<uint8_t>& data, uint32_t offset) {
        memcpy(this, &data[offset], sizeof(Net_slave_health_snapshot));
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t offset) {
        memcpy(&data[offset], this, sizeof(Net_slave_health_snapshot));
    }

    void print() {
        printf("---- Health report:\n");
        printf("Pct RAM used: %f\n", ((float)pct_virt_process_ram_used / 10000.0f));
        printf("Pct LAG ticks: %f\n", ((float)pct_good_vs_lag_ticks / 10000.0f));
        printf("Avg Idle time: %ld\n", avg_tick_idle_time);
        printf("Pct CPU Load: %f\n", 0.0f);
    }
};

/// <summary>
/// Message that reports the health of the slave node
/// How many sessions are filled and how many more we can allow
/// </summary>
struct Net_from_slave_report_health {
    uint8_t type;
    uint32_t num_alive_sessions;
    uint32_t num_sessions_free;
    uint16_t cpu_load;
    uint32_t mem_used_mb;
    uint32_t mem_free_mb;
};

/// <summary>
/// A signal that instructs the master node to keep the session alive in memory
/// When the host has disconnected this message will stop
/// </summary>
struct Net_from_slave_keepalive_sessions {
    uint8_t type;
    uint32_t num_sessions;
    std::vector<uint32_t> session_ids;

    Net_from_slave_keepalive_sessions() {
        type = (uint8_t)MsgType::NetFromSlaveToMasterKeepaliveSession;
        num_sessions = 0;
    }

    Net_from_slave_keepalive_sessions(const std::vector<uint8_t>& data) {
        memcpy(&type, &data[0], sizeof(uint8_t));
        memcpy(&num_sessions, &data[sizeof(uint8_t)], sizeof(uint32_t));

        if (num_sessions > 10000000) { // if its larger than 1000000 sessions, somethings really odd
            printf("[NET-FROM-SLAVE-KEEPALIVE-SESSIONS][ERROR][Num sessions too large]\n");
            return;
        }

        session_ids.resize(num_sessions);
        memcpy(&session_ids[0], &data[sizeof(uint8_t) + sizeof(uint32_t)], sizeof(uint32_t) * num_sessions);
    }
};

/// <summary>
/// Registers the player on the master node so friends can find the player
/// </summary>
struct Net_authenticate_player {
    uint8_t type;
    uint64_t client_password;
    char username[64];
    uint16_t avatar[32];

    
    Net_authenticate_player() {
        type = (uint8_t)MsgType::NetAuthenticatePlayer;
        client_password = 0;
        memset(username, 0, 64);
        memset(avatar, 0, 32 * sizeof(uint16_t));
    }

    Net_authenticate_player(const std::vector<uint8_t>& data) {
        memcpy(this, &data[0], sizeof(Net_authenticate_player));
    }
};


/// <summary>
/// Gets a summary configuration to the slave node so players can connect to it
/// Can also be fetched from the slave node itself
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


//// ########## END MASTER NODE PACKETS ############# ////

//// ########## SLAVE NODE PACKETS ################## ////

/// ---------- Lobby/connection packets --------------- //

struct Net_player_request_slave_config {
    uint8_t type;

    Net_player_request_slave_config() {
        type = (uint8_t)MsgType::NetPlayerRequestSlaveConfig;
    }
};

/// <summary>
/// Not sure when we need this....
/// </summary>
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

    void set_buffer_fast(std::vector<uint8_t>& data, uint32_t offset) {
        memcpy(&data[offset], this, sizeof(Net_player));
    }
};

/// <summary>
/// When the player marked as owner
/// </summary>
struct Net_player_set_gamerule_int {
    uint8_t type;
    uint16_t rule_id;
    uint16_t rule_value;

    Net_player_set_gamerule_int()
        : type((uint8_t)MsgType::NetPlayerSetGameRuleInt),
          rule_id(0),
          rule_value(0) { }

    Net_player_set_gamerule_int(const std::vector<uint8_t>& data, uint32_t offset) {
        memcpy(this, &data[offset], sizeof(Net_player_set_gamerule_int));
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t offset) {
        memcpy(&data[offset], &type, sizeof(Net_player_set_gamerule_int));
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


/// --------- End lobby/connection packets ------------ //

/// ---------- Begin Ingame Packets ------------------- //

/// <summary>
/// this is the struct we use for the snapshot
/// which is a lightweight struct that tells
/// all relevant information about the player
/// </summary>
struct Net_snapshot_player {
    uint8_t type;
    uint16_t entity_id;
    uint8_t hp;
    uint8_t flags;

    Net_snapshot_player() {
        type = (uint8_t)MsgType::NetSnapshotPlayer;
        entity_id = 0;
        hp = 0;
        flags = 0;
    }

    Net_snapshot_player(const std::vector<uint8_t>& data, uint32_t offset) {
        memcpy(this, &data[offset], sizeof(Net_snapshot_player));
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t offset) {
        memcpy(&data[offset], this, sizeof(Net_snapshot_player));
    }
};

/// <summary>
/// Player position and rotation
/// </summary>
struct Net_pos {
    uint8_t     type;
    uint8_t     entity;
    uint16_t    pos[3];
    uint8_t     rot[7];

    Net_pos() {
        type = (uint8_t)MsgType::PlayerPos;
        entity = 0;
    }

    Net_pos(const std::vector<uint8_t>& data, uint32_t offset) {
        memcpy(this, &data[offset], sizeof(Net_pos));
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t offset) {
        memcpy(&data[offset], this, sizeof(Net_pos));
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


/// ------- End ingame packets ---------- ///

#pragma pack(pop)