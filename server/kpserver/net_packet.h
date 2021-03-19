#pragma once

#include <stdint.h>
#include <memory>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#pragma pack(push, 1)

/// The packet we send to clients as well as receive

enum class MsgType {
    None = 0,
    NetPlayerPos,
    NetOtherPos,
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
    NetPlayerHostSessionResponse,
    NetPlayerMasterJoinPrivateSessionRequest,
    NetPlayerMasterJoinPrivateSessionResponse,
    NetPlayerSlaveJoinPrivateSessionRequest,
    NetPlayerSlaveJoinPrivateSessionResponse,
    NetPlayerSlaveJoinPublicSessionRequest,
    NetPlayerSlaveJoinPublicSessionResponse,
    NetPlayerSlaveListSessionsRequest,
    NetPlayerSlaveListSessionsResponse,
    NetPlayerHasJoinedSession,
    NetPlayerHasLeftSession,
    NetPlayerLeaveSessionRequest,
    NetPlayerJoinNotify,
    NetPlayerLeaveSession,
    NetPlayerSession,
    NetError, // 28
    NetSuccess, // 29
    NetSlaveHealthReport,
    NetPlayerSetGameRuleInt,
    NetGameRuleUpdated,
    NetUDPEstablish,
    NetUDPClientConnectionInfo,
    NetPlayerChat,
    NetPlayerStartGameSessionRequest,
    NetPlayerStartGameSessionResponse,
    NetGameSessionWillStart,
    NetPlayerSyncTimeRequest,
    NetPlayerSyncTimeResponse,
    NetSessionSnapshot, // <- the main session snapshot that drives the state of the game
    NetSessionWorldInit,
    NetGameTransformsSnapshot,
    NetGameConfig,
    NetGameSessionHasStarted,
    NetGameSessionHasEnded,
    NetPlayerSetItemStateRequest,
    NetPlayerSetItemStateResponse,
    NetSceneItemStateChanged
};

enum class NetErrorType {
    None = 0,
    SessionNotFound,
    NoAvailableSessions,
    Banned,
    AlreadyHaveSession,
    SessionIsFull,
    NoAvailableSlaves,
    InvalidPassword,
    NoSessionsToList
};

enum class NetSuccessType {
    None = 0,
    Authentication,
    GameRuleOK
};

enum class NetMasterToSlaveCommand {
    None = 0,
    ReportHealth
};

//// ################ COMMON PACKETS ################### ////

struct Net_success {
    uint8_t     type;
    uint8_t     msg;

    Net_success(NetSuccessType success_msg) : 
        type((uint8_t)MsgType::NetSuccess), 
        msg((uint8_t)success_msg) { }
};

struct Net_error {
    uint8_t     type;
    uint8_t     error;

    Net_error(NetErrorType error_type) {
        type = (uint8_t)MsgType::NetError;
        error = (uint8_t)error_type;
    }

};


/// <summary>
/// Sent from player to slave in order to map the client UDP connection to a Net_client
/// </summary>
struct Net_Udp_establish {
    uint8_t     type;
    uint16_t    code;
    uint32_t    client_id;

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
    uint8_t     type;
    uint16_t    code;
    uint32_t    client_id;
    uint16_t    port;
    char        ip[64];

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
    uint8_t     type;
    uint32_t    slave_id;
    uint64_t    master_password;

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
    uint8_t     type;
    uint32_t    player_id;
    char        username[64];
    uint16_t    avatar[32];

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
/// Send out to all players a signal that indicates that the game will start at the given server time
/// </summary>
struct Net_game_session_will_start {
    uint8_t type;
    uint8_t ok;
    uint64_t start_timestamp;

    Net_game_session_will_start() : type((uint8_t)MsgType::NetGameSessionWillStart) {
    
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t off) {
        memcpy(&data[off], this, sizeof(Net_game_session_will_start));
    }
};

struct Net_game_session_has_started {
    uint8_t type;
    uint8_t ok;
    uint64_t start_timestamp;

    Net_game_session_has_started() : type((uint8_t)MsgType::NetGameSessionHasStarted) {

    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t off) {
        memcpy(&data[off], this, sizeof(Net_game_session_has_started));
    }
};

struct Net_game_session_has_ended {
    uint8_t type;
    uint8_t ok;

    Net_game_session_has_ended() : type((uint8_t)MsgType::NetGameSessionHasEnded) {

    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t off) {
        memcpy(&data[off], this, sizeof(Net_game_session_has_ended));
    }
};


struct Net_scene_item_state_updated {
    uint8_t type;
    uint16_t id;
    uint8_t state;

    Net_scene_item_state_updated() : type((uint8_t)MsgType::NetSceneItemStateChanged), id(0), state(0) {}
};

/// <summary>
/// Gives details of the game state
/// Mission updates, doors etc
/// </summary>
struct Net_session_snapshot {
    uint8_t     type;
    uint32_t    session_timestamp;
    uint8_t     items[64];

    Net_session_snapshot() : type((uint8_t)MsgType::NetSessionSnapshot), session_timestamp(0) {}
};

/// <summary>
/// Gives the initial state of the world
/// </summary>
struct Net_session_world_init {
    uint8_t type;
    

    Net_session_world_init() : type((uint8_t)MsgType::NetSessionWorldInit) {}
};

struct Net_player_sync_time_request {
    uint8_t type;
    uint64_t t0; // the time at the client

    Net_player_sync_time_request() : type(((uint8_t)MsgType::NetPlayerSyncTimeRequest)) {}


    Net_player_sync_time_request(const std::vector<uint8_t>& data, uint32_t off) {
        memcpy(this, &data[off], sizeof(Net_player_sync_time_request));
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t off) {
        memcpy(&data[off], this, sizeof(Net_player_sync_time_request));
    }
};

struct Net_player_sync_time_response {
    uint8_t type;
    uint64_t t0;
    uint64_t t1; // the time at the server
    uint32_t session_time;

    Net_player_sync_time_response() : type(((uint8_t)MsgType::NetPlayerSyncTimeResponse)) {}

    void set_buffer(std::vector<uint8_t>& data, uint32_t off) {
        memcpy(&data[off], this, sizeof(Net_player_sync_time_response));
    }
};


/// <summary>
/// When player (who owns the session) starts the game
/// </summary>
struct Net_player_start_game_session_request {
    uint8_t type;

    Net_player_start_game_session_request() : type((uint8_t)MsgType::NetPlayerStartGameSessionRequest) {}

    Net_player_start_game_session_request(const std::vector<uint8_t>& data, uint32_t off) {
        memcpy(this, &data[off], sizeof(Net_player_start_game_session_request));
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t off) {
        memcpy(&data[off], this, sizeof(Net_player_start_game_session_request));
    }
};

struct Net_player_set_item_state_request {
    uint8_t type;
    uint16_t id;
    uint8_t state;
    uint8_t on;

    Net_player_set_item_state_request() : type((uint8_t)MsgType::NetPlayerSetItemStateRequest) {}

    Net_player_set_item_state_request(const std::vector<uint8_t>& data, uint32_t off) {
        memcpy(this, &data[off], sizeof(Net_player_set_item_state_request));
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t off) {
        memcpy(&data[off], this, sizeof(Net_player_set_item_state_request));
    }
};

struct Net_player_set_item_state_response {
    uint8_t type;
    uint8_t success;
    uint16_t id;

    Net_player_set_item_state_response() : type((uint8_t)MsgType::NetPlayerSetItemStateResponse) {}

    Net_player_set_item_state_response(const std::vector<uint8_t>& data, uint32_t off) {
        memcpy(this, &data[off], sizeof(Net_player_set_item_state_response));
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t off) {
        memcpy(&data[off], this, sizeof(Net_player_set_item_state_response));
    }
};

struct Net_player_start_game_session_response {
    uint8_t type;
    uint8_t ok;
    int32_t start_timestamp;

    Net_player_start_game_session_response() : type((uint8_t)MsgType::NetPlayerStartGameSessionResponse) {
        ok = 0;
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t off) {
        memcpy(&data[off], this, sizeof(Net_player_start_game_session_response));
    }
};

/// <summary>
/// When a player wants to host a game
/// </summary>
struct Net_player_host_session_request {
    uint8_t     type;
    uint8_t     is_private; // 0 -> no, 1 -> yes

    Net_player_host_session_request() {
        type = (uint8_t)MsgType::NetPlayerHostSessionRequest;
        is_private = 0;
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
struct Net_player_host_session_response {
    uint8_t     type;
    char        code[8];
    uint32_t    session_id;
    uint8_t     max_players;

    Net_player_host_session_response() {
        type = (uint8_t)MsgType::NetPlayerHostSessionResponse;
    }

    Net_player_host_session_response(const std::vector<uint8_t>& data, uint32_t off) {
        memcpy(this, &data[off], sizeof(Net_player_host_session_response));
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t off) {
        memcpy(&data[off], this, sizeof(Net_player_host_session_response));
    }
};

/// <summary>
/// When a player wants to join a session
/// </summary>
struct Net_player_master_join_private_session_request {
    uint8_t     type;
    char        code[8];

    Net_player_master_join_private_session_request() {
        type = (uint8_t)MsgType::NetPlayerMasterJoinPrivateSessionRequest;
        memset(code, 0, 8);
    }

    Net_player_master_join_private_session_request(const std::vector<uint8_t>& data, uint32_t off) {
        memcpy(this, &data[off], sizeof(Net_player_master_join_private_session_request));
    }
};

/// <summary>
/// This is the request we send to the slave
/// </summary>
struct Net_player_slave_join_private_session_request {
    uint8_t     type;
    char        code[8];

    Net_player_slave_join_private_session_request() {
        type = (uint8_t)MsgType::NetPlayerSlaveJoinPrivateSessionRequest;
        memset(code, 0, 8);
    }

    Net_player_slave_join_private_session_request(const std::vector<uint8_t>& data, uint32_t off) {
        memcpy(this, &data[off], sizeof(Net_player_slave_join_private_session_request));
    }
};

struct Net_player_slave_join_public_session_request {
    uint8_t     type;
    uint32_t    session_id;

    Net_player_slave_join_public_session_request() {
        type = (uint8_t)MsgType::NetPlayerSlaveJoinPublicSessionRequest;
        session_id = 0;
    }

    Net_player_slave_join_public_session_request(const std::vector<uint8_t>& data, uint32_t off) {
        memcpy(this, &data[off], sizeof(Net_player_slave_join_public_session_request));
    }
};

struct Net_player_slave_join_public_session_response {
    uint8_t     type;
    uint32_t    session_id;

    Net_player_slave_join_public_session_response() {
        type = (uint8_t)MsgType::NetPlayerSlaveJoinPublicSessionResponse;
    }

    Net_player_slave_join_public_session_response(const std::vector<uint8_t>& data, uint32_t off) {
        memcpy(this, &data[off], sizeof(Net_player_slave_join_public_session_response));
    }
};


struct Net_player_slave_join_private_session_response {
    uint8_t     type;
    uint32_t    session_id;

    Net_player_slave_join_private_session_response() {
        type = (uint8_t)MsgType::NetPlayerSlaveJoinPrivateSessionResponse;
    }

    Net_player_slave_join_private_session_response(const std::vector<uint8_t>& data, uint32_t off) {
        memcpy(this, &data[off], sizeof(Net_player_slave_join_private_session_response));
    }
};

/// <summary>
/// When we want to join a private session, the master will only reply
/// with the connection details to the slave which is hosting the session
/// it is then up to the client to authenticate with the slave
/// and request to join a private game
/// </summary>
struct Net_player_master_join_private_session_response {
    uint8_t     type;

    uint32_t    slave_id;
    uint16_t    tcp_port;
    uint16_t    udp_port;
    char        ip[64];

    Net_player_master_join_private_session_response() {
        type = (uint8_t)MsgType::NetPlayerMasterJoinPrivateSessionResponse;
    }

    Net_player_master_join_private_session_response(const std::vector<uint8_t>& data, uint32_t off) {
        memcpy(this, &data[off], sizeof(Net_player_master_join_private_session_response));
    }
};

/// <summary>
/// Send this when a player has joined a session
/// Also send it to all players inside the session
/// </summary>
struct Net_player_has_joined_session {
    uint8_t     type;
    uint8_t     player_short_id;
    uint32_t    session_id;
    char        username[64];
    uint16_t    avatar[32];

    Net_player_has_joined_session(uint8_t player_short_id_, uint32_t session_id_, const char* username_) {
        type = (uint8_t)MsgType::NetPlayerHasJoinedSession;
        player_short_id = player_short_id_;
        session_id = session_id_;
        memcpy(username, username_, 64);
        memset(avatar, 0, 32 * sizeof(uint16_t));
    }

    Net_player_has_joined_session(const std::vector<uint8_t>& data, uint32_t off) {
        memcpy(this, &data[off], sizeof(Net_player_has_joined_session));
    }
};

/// <summary>
/// A broadcast message to send to other players in the session
/// when a player has left the session
/// </summary>
struct Net_player_has_left_session {
    uint8_t     type;
    uint8_t     player_index;
    uint32_t    player_id;
    uint32_t    session_id;

    Net_player_has_left_session(uint8_t player_index_, uint32_t player_id_, uint32_t session_id_) {
        type = (uint8_t)MsgType::NetPlayerHasLeftSession;
        player_index = player_index_;
        session_id = session_id_;
        player_id = player_id_;
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t off) {
        memcpy(&data[off], this, sizeof(Net_player_has_left_session));
    }
};

/// <summary>
/// When a player leaves a session
/// </summary>
struct Net_player_leave_session_request {
    uint8_t     type;
    uint32_t    player_id;
    uint32_t    session_id;

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
    uint8_t     type;

    Net_player_slave_node_request() {
        type = (uint8_t)MsgType::NetPlayerSlaveNodeRequest;
    }

};

struct Net_player_slave_node_response {
    uint8_t     type;
    uint32_t    slave_id;
    uint16_t    tcp_port;
    uint16_t    udp_port;
    char        ip[64];

    Net_player_slave_node_response() {
        type = (uint8_t)MsgType::NetPlayerSlaveNodeResponse;
    }

    Net_player_slave_node_response(uint32_t slave_id, const char* ip_, uint16_t tcp_port, uint16_t udp_port) 
        : slave_id(slave_id), tcp_port(tcp_port), udp_port(udp_port), type((uint8_t)MsgType::NetPlayerSlaveNodeResponse)
    {
        memcpy(ip, ip_, 64);
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
    uint8_t     type;
    uint8_t     command; // 0 -> report keepalive, 1 -> shutdown sessions, 2 -> process shutdown

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
    uint8_t     type;

    uint16_t    pct_virt_process_ram_used;
    uint16_t    pct_good_vs_lag_ticks;
    int64_t     avg_tick_idle_time;
    uint16_t    pct_cpu_load_process;
    uint16_t    num_connected_players;

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
    uint8_t     type;
    uint32_t    num_alive_sessions;
    uint32_t    num_sessions_free;
    uint16_t    cpu_load;
    uint32_t    mem_used_mb;
    uint32_t    mem_free_mb;
};

/// <summary>
/// Syncs the state of the session with the master
/// </summary>
struct Net_from_slave_sync_session {
    uint8_t     type;
    uint32_t    session_id;
    uint8_t     num_players;
    char        code[8];

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
    uint8_t     type;
    uint64_t    client_password;
    char        username[64];
    uint16_t    avatar[32];

    
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
    uint8_t     type;
    uint32_t    node_id;
    uint16_t    tcp_port;
    uint16_t    udp_port;
    char        hostname[64];
    char        ip[64];

    Net_slave_config() 
    :   type((uint8_t)MsgType::NetSlaveConfig), 
        tcp_port(0),
        udp_port(0),
        node_id(0) {
        memset(hostname, 0, 64);
        memset(ip, 0, 32);
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
struct Net_player_set_gamerule_int_request {
    uint8_t     type;
    uint16_t    rule_id;
    uint16_t    rule_value;

    Net_player_set_gamerule_int_request()
        : type((uint8_t)MsgType::NetPlayerSetGameRuleInt),
          rule_id(0),
          rule_value(0) { }

    Net_player_set_gamerule_int_request(const std::vector<uint8_t>& data, uint32_t off) {
        memcpy(this, &data[off], sizeof(Net_player_set_gamerule_int_request));
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t off) {
        memcpy(&data[off], &type, sizeof(Net_player_set_gamerule_int_request));
    }
};

struct Net_game_config {
    uint8_t type;
    uint16_t num_rules;

    std::vector<uint16_t> rules;

    Net_game_config() : type((uint8_t)MsgType::NetGameConfig) {}

    void set_buffer(std::vector<uint8_t>& data, uint32_t off) {
        memcpy(&data[off], &type, 3);
        memcpy(&data[off + 3], &rules[0], sizeof(uint16_t) * num_rules);
    }

    size_t size() const {
        return (size_t)3 + (num_rules * 4);
    }
};

struct Net_game_rule_updated {
    uint8_t type;
    uint16_t rule_id;
    uint16_t rule_value;

    Net_game_rule_updated()
        : type((uint8_t)MsgType::NetGameRuleUpdated),
        rule_id(0),
        rule_value(0) { }

    Net_game_rule_updated(const std::vector<uint8_t>& data, uint32_t off) {
        memcpy(this, &data[off], sizeof(Net_game_rule_updated));
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t off) {
        memcpy(&data[off], &type, sizeof(Net_game_rule_updated));
    }
};


/// <summary>
/// Lists all players in a session
/// </summary>
struct Net_slave_session_players_list {
    uint8_t     type;
    uint8_t     num_players;

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
    uint8_t     type;
    uint32_t    session_id;
    uint8_t     max_players;
    uint8_t     num_players;

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
/// Lists all public sessions in the node
/// </summary>
struct Net_player_slave_list_sessions_request {
    uint8_t     type;

    Net_player_slave_list_sessions_request() {
        type = (uint8_t)MsgType::NetPlayerSlaveListSessionsRequest;
    }
};

/// <summary>
/// This is just a sub item to a Net_player_slave_list_sessions_response
/// which gives a short summary of available public sessions
/// </summary>
struct Net_slave_session_item {
    uint32_t    session_id;
    uint8_t     num_players;
    uint8_t     max_players;
    char        name[32];

    Net_slave_session_item(uint32_t session_id_, const char* name_, uint8_t num_players_, uint8_t max_players_)
    {
        memcpy(name, name_, 32);
        session_id = session_id_;
        num_players = num_players_;
        max_players = max_players_;
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t off) { 
        memcpy(&data[off], this, sizeof(Net_slave_session_item));
    }
};

/// <summary>
/// Gives a list of sessions to a player who is connected on a 
/// slave node
/// </summary>
struct Net_player_slave_list_sessions_response {
    uint8_t     type;
    uint8_t     num_sessions;
    std::vector<Net_slave_session_item> sessions;

    Net_player_slave_list_sessions_response() {
        type = (uint8_t)MsgType::NetPlayerSlaveListSessionsResponse;
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t off) {
        num_sessions = (uint8_t)sessions.size();
        memcpy(&data[off], this, sizeof(uint8_t) + sizeof(uint8_t));
        uint32_t offset = off + sizeof(uint8_t) + sizeof(uint8_t);

        for (int i = 0; i < sessions.size(); ++i) {
            sessions[i].set_buffer(data, offset);
            offset += sizeof(Net_slave_session_item);
        }
    }

    uint32_t header_size() const {
        return sizeof(uint8_t) + sizeof(uint8_t);
    }

    uint32_t data_size() const {
        return sizeof(Net_slave_session_item) * num_sessions;
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
    uint8_t     type;
    uint16_t    entity_id;
    uint8_t     hp;
    uint8_t     flags;

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
    uint8_t     type;
    uint16_t    len;
    char*       message;

    Net_chat() : type((uint8_t)MsgType::NetPlayerChat), len(0), message(0) {
    
    }
};

#define FLOAT_PRECISION_MULT 32767.0f

/// <summary>
/// All the players positions
/// </summary>
struct Net_game_transforms_snapshot {
    uint8_t type;

    uint8_t num_players;
    uint8_t num_items;

    std::vector<uint8_t> player_transforms;
    std::vector<uint8_t> item_transforms;

    Net_game_transforms_snapshot() : type((uint8_t)MsgType::NetGameTransformsSnapshot) {
        num_players = 0;
        num_items = 0;
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t offset) {
        memcpy(&data[offset], this, 3);
        memcpy(&data[offset + 3], &player_transforms[0], player_transforms.size());
        //memcpy(&data[offset + 3 + player_transforms.size()], &item_transforms[0], item_transforms.size());
    }

    uint32_t size() const {
        return 3 + player_transforms.size() * (6 + 7);
    }
};

/// <summary>
/// Player position and rotation
/// </summary>
struct Net_pos {
    uint8_t     type;
    uint8_t     player_index;
    uint16_t    pos[3];
    uint8_t     rot[7];

    Net_pos() {
        type = (uint8_t)MsgType::NetPlayerPos;
        player_index = 0;
    }

    Net_pos(const std::vector<uint8_t>& data, uint32_t off) {
        memcpy(this, &data[off], sizeof(Net_pos));
    }

    void set_buffer(std::vector<uint8_t>& data, uint32_t off) {
        memcpy(&data[off], this, sizeof(Net_pos));
    }

    void print() {
        printf("[MSG-POS][E: %d][x: %d][y: %d][z: %d]\n", player_index, pos[0], pos[1], pos[2]);
        printf("[MSG-POS][DEBUG][x: %f][y: %f][z: %f]\n", ((float)pos[0]) / 100.0f, ((float)pos[1]) / 100.0f, ((float)pos[2]) / 100.0f);
    }

    glm::vec3 to_vec3() const {
        return glm::vec3(((float)pos[0]) / 100.0f, ((float)pos[0]) / 100.0f, ((float)pos[0]) / 100.0f);
    }

    glm::quat to_quat() const {
        uint8_t maxindex = rot[0];

        if (maxindex >= 4 && maxindex <= 7)
        {
            float x = (maxindex == 4) ? 1.0f : 0.0f;
            float y = (maxindex == 5) ? 1.0f : 0.0f;
            float z = (maxindex == 6) ? 1.0f : 0.0f;
            float w = (maxindex == 7) ? 1.0f : 0.0f;

            return glm::quat(x, y, z, w);
        }

        uint16_t v[3];

        memcpy(&v[0], &rot[1], sizeof(uint16_t));
        memcpy(&v[1], &rot[3], sizeof(uint16_t));
        memcpy(&v[2], &rot[5], sizeof(uint16_t));

        // Read the other three fields and derive the value of the omitted field
        float a = (float)v[0] / FLOAT_PRECISION_MULT;
        float b = (float)v[1] / FLOAT_PRECISION_MULT;
        float c = (float)v[2] / FLOAT_PRECISION_MULT;
        float d = sqrt(1.0f - (a * a + b * b + c * c));

        if (maxindex == 0)
        {
            return glm::quat(d, a, b, c);
        }
        else if (maxindex == 1)
        {
            return glm::quat(a, d, b, c);
        }
        else if (maxindex == 2)
        {
            return glm::quat(a, b, d, c);
        }

        return glm::quat(a, b, c, d);
    }

    
};

struct Net_packet {
    Net_packet(uint32_t size);
    virtual ~Net_packet();

    std::vector<uint8_t> data;
};


/// ------- End ingame packets ---------- ///

#pragma pack(pop)