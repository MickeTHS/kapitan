#include "net_session.h"
#include "net_packet.h"
#include "tcp_server.h"
#include "udp_server.h"
#include "net_client.h"

#include <cstdlib>
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <time.h>

uint32_t Net_session::__SESSION_ID_COUNTER = 1;

Net_session::Net_session(uint8_t max_players, uint32_t keepalive_seconds, Tcp_server* tcp_server, Udp_server* udp_server)
    :   _id(__SESSION_ID_COUNTER++),
        _max_players(max_players), 
        _tcp_server(tcp_server),
        _udp_server(udp_server),
        _owner(NULL),
        _keepalive_seconds(keepalive_seconds),
        _is_private(false),
        _num_players(0),
        _session_timestamp(0),
        _world(std::make_unique<World_instance>()),
        _game_running(false),
        _game_starting(false),
        _time_since_snapshot(0) {

    _players.resize(max_players);
    _data_buffer.resize(2000);
    game_config.rules.resize(4 * (int)GameRule::MaxRules);
    game_config.rules[GameRule::PlayerMovementSpeed] = 1000; // mm per second
    game_config.rules[GameRule::GameSeconds] = 60; // game length in seconds

    
    memset(session_code, 0, 16);
    generate_session_code();
}

Net_session::~Net_session() {}

void Net_session::keepalive() {
    _last_keepalive = std::chrono::high_resolution_clock::now();
}

bool Net_session::is_full() const {
    uint16_t num_players = 0;

    for (auto& player : _players) {
        if (player.is_set) {
            num_players++;
        }
    }

    return num_players == _max_players;
}

void Net_session::disconnect(uint32_t client_id) {
    for (int i = 0; i < _players.size(); ++i) {
        if (_players[i].net_client_id == client_id) {
            _players.erase(_players.begin() + i);
            _num_players--;
            return;
        }
    }
}

void Net_session::set_private(bool priv) {
    _is_private = priv;
}

bool Net_session::is_private() const {
    return _is_private;
}

void Net_session::generate_session_code() {
    _id = __SESSION_ID_COUNTER++;

    char alfa[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ\0";
    int len = strlen(alfa);

    srand(time(NULL));

    for (int i = 0; i < 8; ++i) {
        session_code[i] = alfa[rand() % len];
    }

    session_code_hash = mmh::Hash_key(session_code);
}

bool Net_session::is_empty() const {
    return _num_players == 0;
}

bool Net_session::is_old(std::chrono::time_point<std::chrono::high_resolution_clock>& now) const {
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - _last_keepalive).count();

    if (duration > _keepalive_seconds) {
        return true;
    }

    return false;
}

void Net_session::set_num_players(uint8_t num_players) {
    _num_players = num_players;
}

uint8_t Net_session::get_num_players() const {
    return _num_players;
}

uint8_t Net_session::get_max_players() const {
    return _max_players;
}

void Net_session::remove_player(Net_client* client) {
    bool pull_back = false;

    for (int i = 0; i < _players.size(); ++i) {
        if (_players[i].net_client_id == client->get_id()) {
            _players[i].reset();
            pull_back = true;
            _num_players--;
            continue;
        }

        // make sure that we reassign players in the first slots of the array
        if (pull_back) {
            _players[i-1].assign(_players[i]);
            _players[i].reset();
        }
    }
}

void Net_session::broadcast_udp(void* data, uint32_t len) {
    for (auto& player : _players) {
        player.client_connection->add_udp_data(data, len);
    }
}

void Net_session::broadcast_tcp(void* data, uint32_t len) {
    for (auto& player : _players) {
        player.client_connection->add_tcp_data(data, len);
    }
}


Net_client* Net_session::find_client(Net_client* client) const {
    for (auto& player : _players) {
        if (player.client_connection == client) {
            return player.client_connection;
        }
    }

    return NULL;
}

void Net_session::on_udp_data(const std::vector<uint8_t>& data, int32_t data_len) {
    /*MsgType type = (MsgType)data[0];

    switch(type) {
        case MsgType::PlayerPos:
            printf("[NET-SESSION][UDP-PLAYER-POS]");
    }*/
}

void Net_session::handle_inc_pos(const std::vector<uint8_t>& data) {
    Net_pos pos(data, 0);

    if (_on_pos != nullptr) {
        _on_pos(pos);
    }
}

int32_t Net_session::on_tcp_data(const std::vector<uint8_t>& data, uint32_t offset, int32_t len) {
    // when a packet was unable to be parsed by the
    // net_slave it will enter here to be processed

    // return the amount of data read
    return 0;
}

void Net_session::on_inc_pos(const Net_pos& pos) {
    _world->on_player_pos(pos);
}

void Net_session::set_on_pos(std::function<void(const Net_pos&)> func) {
    _on_pos = func;
}

uint32_t Net_session::get_id() const {
    return _id;
}

bool Net_session::remove_player_and_broadcast(Net_client* client, bool broadcast) {
    
    remove_player(client);

    if (broadcast) {
        Net_player_has_left_session resp(client->info.player_short_id, client->info.client_id, _id);
        broadcast_tcp(&resp, sizeof(Net_player_has_joined_session));
    }

    return true;
}

bool Net_session::add_player_and_broadcast(Net_client* client, bool broadcast, bool is_owner) {
    if (_num_players == _max_players) {
        return false; // its full
    }

    _players[_num_players].assign(_num_players, client->info.client_id, client);
    _num_players++;

    client->set_session_id(_id);
    client->info.session_id = _id;
    

    if (is_owner) {
        _owner = client;
    }

    if (broadcast) {
        Net_player_has_joined_session resp(_num_players, _id, client->info.username);
        _num_players++;
        broadcast_tcp(&resp, sizeof(Net_player_has_joined_session));
    }
    
    return true;
}

int Net_session::read() {
    return _udp_server->read();
}

void Net_session::start_game_in_seconds(int seconds) {
    _game_start_time = std::chrono::high_resolution_clock::now() + std::chrono::seconds(5);
    _game_starting = true;
}

void Net_session::end_game() {
    _game_running = false;
}

uint32_t Net_session::get_time() const {
    if (!_game_running) {
        return 0;
    }

    return _session_timestamp;
}


uint64_t Net_session::get_end_time() const {
    return _game_end_time.time_since_epoch().count();
}

uint64_t Net_session::get_start_time() const {
    return _game_start_time.time_since_epoch().count();
}

bool Net_session::can_game_be_started() const {
    return _num_players > 2;
}

/// <summary>
/// When the game has started (countdown over)
/// sends out the game has started message
/// we might need to add some NPCs and states, maybe randomize something?
/// </summary>
/// <param name="now"></param>
void Net_session::on_game_started(std::chrono::time_point<std::chrono::high_resolution_clock>& now) {
    _game_end_time = std::chrono::high_resolution_clock::now() + std::chrono::seconds(game_config.rules[GameRule::GameSeconds]);
    _last_tick = now;

    Net_game_session_has_started start;
    start.ok = 1;
    start.start_timestamp = now.time_since_epoch().count();

    // notify all clients that a game has started
    for (int i = 0; i < _players.size(); ++i) {
        _players[i].client_connection->add_tcp_data(&start, sizeof(Net_game_session_has_started));
    }
}

/// <summary>
/// Send out the game ended summary
/// </summary>
/// <param name="now"></param>
void Net_session::on_game_ended(std::chrono::time_point<std::chrono::high_resolution_clock>& now) {
    _game_running = false;

    Net_game_session_has_ended end;
    end.ok = 1;
    
    // notify all clients that a game has started
    for (int i = 0; i < _players.size(); ++i) {
        _players[i].client_connection->add_tcp_data(&end, sizeof(Net_game_session_has_ended));
    }
}

void Net_session::on_time_for_snapshot() {
    broadcast_udp(&_snapshot, sizeof(Net_session_snapshot));
}

void Net_session::update_game(const double delta, std::chrono::time_point<std::chrono::high_resolution_clock>& now) {

    if (_game_starting) {
        if (now > _game_start_time) {
        
            _game_starting = false;
            _game_running = true;

            on_game_started(now);
        }
    }
    
    if (!_game_running) {
        return;
    }

    _time_since_snapshot += delta;

    _session_timestamp = (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(now - _game_start_time).count();
    _now = now;

    if (_time_since_snapshot > 1.0f) {
        _time_since_snapshot = 0.0f;

        on_time_for_snapshot();
    }

    if (_game_end_time < _now) {
        on_game_ended(now);
        return;
    }
}

void Net_session::send_udp(std::chrono::time_point<std::chrono::high_resolution_clock>& now) {
    _world->fill_transform();

    _world->data_transforms.set_buffer(_data_buffer, 0);
    uint32_t size = _world->data_transforms.size();

    for (int i = 0; i < _players.size(); ++i) {
        _udp_server->send_client(_players[i].client_connection, _data_buffer, size);
    }
}

bool Net_session::set_game_rule(uint16_t rule_id, uint16_t rule_value) {
    if (rule_id >= GameRule::MaxRules) {
        return false;
    }

    game_config.rules[rule_id] = rule_value;

    return true;
}

void Net_session::msg_item_set(Net_client* client, const Net_player_set_item_state_request& request) {
    Net_player_set_item_state_response resp;
    
    _world->set_item_state(request, resp);
    _world->set_item_snapshot_data(_snapshot.items);
}
