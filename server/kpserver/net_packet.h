#pragma once

#include <stdint.h>
#include <memory>
#include <vector>
#include <stdio.h>
#include <string.h>

#pragma pack(push, 1)

/// The packet we send to clients as well as receive

enum class MsgType {
    None = -1,
    GroupConfig = 0,
    NetPlayerPos = 1,
    NetOtherPos = 2,
    NetPlayerSlaveNodeRequest,
    NetPlayerSlaveNodeResponse,
    NetSlaveConfig,
    NetSlaveSessionSummary,
    NetSnapshotPlayer,
    NetAuthenticatePlayer,
    NetAuthenticateSlave,
    NetFromSlaveSyncSession,
    NetFromMasterToSlaveCommand,
    NetPlayerHostSessionRequest,
    NetPlayerHostSessionResponseOk,
    NetPlayerHostSessionResponseReject,
    NetPlayerJoinSessionRequest,
    NetPlayerJoinSessionResponse,
    NetPlayerLeaveSessionRequest,
    NetPlayerJoinNotify,
    NetPlayerLeaveSession,
    NetPlayerSession,
    NetError,
    NetSuccess,
    NetSlaveHealthReport,
    NetPlayerSetGameRuleInt,
    NetUDPEstablish,
    NetUDPClientConnectionInfo,
    NetPlayerChat
};

enum class NetErrorType {
    None = 0,
    SessionNotFound,
    NoAvailableSessions,
    Banned,
    AlreadyHaveSession,
    SessionIsFull,
    NoAvailableSlaves
};

enum class NetSuccessType {
    None = 0
};

enum class NetMasterToSlaveCommand {
    None = 0,
    ReportHealth
};

//// ########## COMMON PACKETS ################### ////

struct Net_success {
    uint8_t type;
    uint8_t msg;

    Net_success(NetSuccessType success_msg) : type((uint8_t)MsgType::NetSuccess), msg((uint8_t)success_msg) { }
};

struct Net_error {
    uint8_t type;
    uint8_t error;

    Net_error(NetErrorType error_type) {
        type = (uint8_t)MsgType::NetError;
        error = (uint8_t)error_type;
    }

};


/// <summary>
/// Sent from player to slave in order to map the client UDP connection to a Net_client
/// </summary>
struct Net_Udp_establish {
    uint8_t type;
    uint16_t code;
    uint32_t client_id;

    Net_Udp_establish() : type((uint8_t)MsgType::NetUDPEstablish), code(0), client_id(0) {
    
    }

    Net_Udp_establish(const std::vector<uint8_t>& data, uint32_t off) {
        memcpy(this, &data[off], sizeof(Net_Udp_establish));
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t off) {
        memcpy(&data[off], this, sizeof(Net_Udp_establish));
    }
};

/// <summary>
/// Notify the client that we have a reserved connection slot for UDP data
/// using the credentials listed below
/// </summary>
struct Net_Udp_client_connection_info {
    uint8_t type;
    uint16_t code;
    uint32_t client_id;
    uint16_t port;
    char ip[64];

    Net_Udp_client_connection_info() : type((uint8_t)MsgType::NetUDPClientConnectionInfo), code(0), client_id(0), port(0) {
        memset(ip, 0, 64);
    }

    Net_Udp_client_connection_info(const std::vector<uint8_t>& data, uint32_t off) {
        memcpy(this, &data[off], sizeof(Net_Udp_client_connection_info));
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t off) {
        memcpy(&data[off], this, sizeof(Net_Udp_client_connection_info));
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

    Net_authenticate_slave(const std::vector<uint8_t>& data, uint32_t off) {
        memcpy(this, &data[off], sizeof(Net_authenticate_slave));
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t off) {
        memcpy(&data[off], this, sizeof(Net_authenticate_slave));
    }
};

struct Net_session_player_packet {
    uint8_t     type;
    uint32_t    player_id;
    char        username[64];
    uint16_t    avatar[32];

    Net_session_player_packet() : type((uint8_t)MsgType::NetPlayerSession), player_id(0) {
        
    }

    Net_session_player_packet(const std::vector<uint8_t>& data, uint32_t off) {
        memcpy(this, &data[off], sizeof(Net_session_player_packet));
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t off) {
        memcpy(&data[off], this, sizeof(Net_session_player_packet));
    }

    void from_buffer(const std::vector<uint8_t>& data, uint32_t off) {
        memcpy(this, &data[off], sizeof(Net_session_player_packet));
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

    Net_player_join_notify(const std::vector<uint8_t>& data, uint32_t off) {
        memcpy(this, &data[off], sizeof(Net_player_join_notify));
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t off) {
        memcpy(&data[off], &type, sizeof(Net_player_join_notify));
    }
};


/// <summary>
/// When a player wants to host a game
/// </summary>
struct Net_player_host_session_request {
    uint8_t type;
    
    Net_player_host_session_request() {
        type = (uint8_t)MsgType::NetPlayerHostSessionRequest;
    }

    Net_player_host_session_request(const std::vector<uint8_t>& data, uint32_t off) {
        memcpy(this, &data[off], sizeof(Net_player_host_session_request));
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t off) {
        memcpy(&data[off], this, sizeof(Net_player_host_session_request));
    }
};


/// <summary>
/// The response sent back to the client regarding the request to to host a session
/// </summary>
struct Net_player_host_session_response_ok {
    uint8_t     type;
    char        code[16];
    uint32_t    session_id;
    uint8_t     max_players;

    Net_player_host_session_response_ok() {
        type = (uint8_t)MsgType::NetPlayerHostSessionResponseOk;
    }

    Net_player_host_session_response_ok(const std::vector<uint8_t>& data, uint32_t off) {
        memcpy(this, &data[off], sizeof(Net_player_host_session_response_ok));
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t off) {
        memcpy(&data[off], this, sizeof(Net_player_host_session_response_ok));
    }
};

/// <summary>
/// When a player wants to join a session
/// </summary>
struct Net_player_join_session_request {
    uint8_t type;
    char code[16];

    Net_player_join_session_request() {
        type = (uint8_t)MsgType::NetPlayerJoinSessionRequest;
        memset(code, 0, 16);
    }

    Net_player_join_session_request(const std::vector<uint8_t>& data, uint32_t off) {
        memcpy(this, &data[off], sizeof(Net_player_join_session_request));
    }
};

/// <summary>
/// Send this when a player has joined a session
/// Also send it to all players inside the session
/// </summary>
struct Net_player_join_session_response {
    uint8_t type;
    uint8_t player_short_id;
    uint32_t session_id;
    char username[64];
    uint16_t avatar[32];

    Net_player_join_session_response(uint8_t player_short_id_, uint32_t session_id_, const char* username_) {
        type = (uint8_t)MsgType::NetPlayerJoinSessionResponse;
        player_short_id = player_short_id_;
        session_id = session_id_;
        strcpy(username, username_);
        memset(avatar, 0, 32 * sizeof(uint16_t));
    }

    Net_player_join_session_response(const std::vector<uint8_t>& data, uint32_t off) {
        memcpy(this, &data[off], sizeof(Net_player_join_session_response));
    }
};

/// <summary>
/// When a player leaves a session
/// </summary>
struct Net_player_leave_session_request {
    uint8_t type;
    uint32_t player_id;
    uint32_t session_id;

    Net_player_leave_session_request() : type((uint8_t)MsgType::NetPlayerLeaveSessionRequest), session_id(0), player_id(0) {}

    Net_player_leave_session_request(const std::vector<uint8_t>& data, uint32_t off) {
        memcpy(this, &data[off], sizeof(Net_player_leave_session_request));
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t off) {
        memcpy(&data[off], this, sizeof(Net_player_leave_session_request));
    }
};



/// <summary>
/// Sent to a master node to get a "good" slave node
/// Should send back a Net_slave_config to the player
/// </summary>
struct Net_player_slave_node_request {
    uint8_t type;

    Net_player_slave_node_request() {
        type = (uint8_t)MsgType::NetPlayerSlaveNodeRequest;
    }

};

struct Net_player_slave_node_response {
    uint8_t type;
    uint32_t slave_id;
    uint16_t tcp_port;
    char ip[64];

    Net_player_slave_node_response() {
        type = (uint8_t)MsgType::NetPlayerSlaveNodeResponse;
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t off) {
        memcpy(&data[off], this, sizeof(Net_player_slave_node_response));
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

    Net_master_to_slave_command(const std::vector<uint8_t>& data, uint32_t off) {
        memcpy(this, &data[off], sizeof(Net_master_to_slave_command));
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t off) {
        memcpy(&data[off], &type, sizeof(uint8_t));
        memcpy(&data[off + sizeof(uint8_t)], &command, sizeof(uint8_t));
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
    uint16_t num_connected_players;

    Net_slave_health_snapshot(float virt_process_ram_used, float good_vs_lag_ticks, int64_t avg_idle_time, float cpu_load, uint16_t num_players) {
        type = (uint8_t)MsgType::NetSlaveHealthReport;
        pct_virt_process_ram_used = (uint16_t)(virt_process_ram_used * 10000.0f);
        pct_good_vs_lag_ticks = (uint16_t)(good_vs_lag_ticks * 10000.0f);
        pct_cpu_load_process = (uint16_t)(cpu_load * 10000.0f);

        avg_tick_idle_time = avg_idle_time;
        num_connected_players = num_players;
    }

    Net_slave_health_snapshot(const std::vector<uint8_t>& data, uint32_t off) {
        memcpy(this, &data[off], sizeof(Net_slave_health_snapshot));
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t off) {
        memcpy(&data[off], this, sizeof(Net_slave_health_snapshot));
    }

    void print() {
        printf("---- Health report:\n");
        printf("Pct RAM used: %f\n", ((float)pct_virt_process_ram_used / 10000.0f));
        printf("Pct LAG ticks: %f\n", ((float)pct_good_vs_lag_ticks / 10000.0f));
        printf("Avg Idle time: %lld\n", avg_tick_idle_time);
        printf("Pct CPU Load: %f\n", 0.0f);
        printf("Num players: %d\n", num_connected_players);
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
/// Syncs the state of the session with the master
/// </summary>
struct Net_from_slave_sync_session {
    uint8_t type;
    uint32_t session_id;
    uint8_t num_players;
    char code[8];

    Net_from_slave_sync_session() {
        type = (uint8_t)MsgType::NetFromSlaveSyncSession;
        session_id = 0;
        num_players = 0;
        memset(code, 0, 8);
    }

    Net_from_slave_sync_session(const std::vector<uint8_t>& data, uint32_t off) {
        memcpy(this, &data[off], sizeof(Net_from_slave_sync_session));
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t off) {
        memcpy(&data[off], this, sizeof(Net_from_slave_sync_session));
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

    Net_authenticate_player(const std::vector<uint8_t>& data, uint32_t off) {
        memcpy(this, &data[off], sizeof(Net_authenticate_player));
    }
};


/// <summary>
/// Gets a summary configuration to the slave node so players can connect to it
/// Can also be fetched from the slave node itself
/// </summary>
struct Net_slave_config {
    uint8_t type;
    uint32_t node_id;
    uint16_t tcp_port;
    uint16_t udp_port;
    char hostname[64];

    Net_slave_config() 
    :   type((uint8_t)MsgType::NetSlaveConfig), 
        tcp_port(0),
        udp_port(0),
        node_id(0) {
        memset(hostname, 0, 64);
    }

    Net_slave_config(const std::vector<uint8_t>& data, uint32_t off) {
        memcpy(this, &data[off], sizeof(Net_slave_config));
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t off) {
        memcpy(&data[off], this, sizeof(Net_slave_config));
    }
};


//// ########## END MASTER NODE PACKETS ############# ////

//// ########## SLAVE NODE PACKETS ################## ////

/// ---------- Lobby/connection packets --------------- //



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

    Net_player_set_gamerule_int(const std::vector<uint8_t>& data, uint32_t off) {
        memcpy(this, &data[off], sizeof(Net_player_set_gamerule_int));
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t off) {
        memcpy(&data[off], &type, sizeof(Net_player_set_gamerule_int));
    }
};


/// <summary>
/// Lists all players in a session
/// </summary>
struct Net_slave_session_players_list {
    uint8_t type;
    uint8_t num_players;

    std::vector<Net_session_player_packet> players;

    void from_buffer(const std::vector<uint8_t>& data, uint32_t off) {
        memcpy(&type, &data[0], sizeof(type));
        memcpy(&num_players, &data[sizeof(type)], sizeof(num_players));

        players.resize(num_players);

        for (int i = 0; i < num_players; ++i) {
            players[i].from_buffer(data, sizeof(type) + sizeof(num_players) + sizeof(Net_session_player_packet) * i);
        }
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t off) {
        memcpy(&data[off], &type, sizeof(type));
        memcpy(&data[off + sizeof(type)], &num_players, sizeof(num_players));

        for (int i = 0; i < num_players; ++i) {
            players[i].set_buffer(data, off + sizeof(type) + sizeof(num_players) + sizeof(Net_session_player_packet) * i);
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

    void from_buffer(const std::vector<uint8_t>& data, uint32_t off) {
        memcpy(&type, &data[off], sizeof(type));
        memcpy(&session_id, &data[off + sizeof(type)], sizeof(session_id));
        memcpy(&max_players, &data[off + sizeof(type) + sizeof(session_id)], sizeof(max_players));
        memcpy(&num_players, &data[off + sizeof(type) + sizeof(session_id) + sizeof(max_players)], sizeof(num_players));
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t off) {
        memcpy(&data[off], &type, sizeof(type));
        memcpy(&data[off + sizeof(type)], &session_id, sizeof(session_id));
        memcpy(&data[off + sizeof(type) + sizeof(session_id)], &max_players, sizeof(max_players));
        memcpy(&data[off + sizeof(type) + sizeof(session_id) + sizeof(max_players)], &num_players, sizeof(num_players));
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

    void set_buffer(std::vector<uint8_t>& data, uint32_t off) {
        memcpy(&data[off], &type, 1);
        memcpy(&data[off + sizeof(uint8_t)], &group_id, sizeof(group_id));
        memcpy(&data[off + sizeof(uint8_t) + sizeof(group_id)], &pos_port, sizeof(pos_port));
        memcpy(&data[off + sizeof(uint8_t) + sizeof(group_id) + sizeof(pos_port)], hostname, 64);
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

    Net_snapshot_player(const std::vector<uint8_t>& data, uint32_t off) {
        memcpy(this, &data[off], sizeof(Net_snapshot_player));
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t off) {
        memcpy(&data[off], this, sizeof(Net_snapshot_player));
    }
};

/// <summary>
/// We will only forward chat messages
/// </summary>
struct Net_chat {
    uint8_t type;
    uint16_t len;
    char* message;

    Net_chat() : type((uint8_t)MsgType::NetPlayerChat), len(0), message(0) {
    
    }
};

/// <summary>
/// Player position and rotation
/// </summary>
struct Net_pos {
    uint8_t     type;
    uint8_t     player_short_id;
    uint16_t    pos[3];
    uint8_t     rot[7];

    Net_pos() {
        type = (uint8_t)MsgType::NetPlayerPos;
        player_short_id = 0;
    }

    Net_pos(const std::vector<uint8_t>& data, uint32_t off) {
        memcpy(this, &data[off], sizeof(Net_pos));
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t off) {
        memcpy(&data[off], this, sizeof(Net_pos));
    }

    void print() {
        printf("[MSG-POS][E: %d][x: %d][y: %d][z: %d]\n", player_short_id, pos[0], pos[1], pos[2]);
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