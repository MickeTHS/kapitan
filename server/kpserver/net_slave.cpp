#include "net_slave.h"

#include "net_packet.h"
#include "process_stats.h"
#include "ini_file.h"
#include "trace.h"

Net_slave::Net_slave(Ini_file* file, Process_stats* stats) 
    :
    _ini_file(file), 
    _stats(stats),
    _num_players(0),
    _tick_check(0) {

    _master.node = _ini_file->get_master();
    _my_node = _ini_file->get_me();

    _next_slave_report = std::chrono::high_resolution_clock::now() - std::chrono::minutes(60);

    _data_buffer.resize(2000);

    _master_connection.set_on_data_callback([&](const std::vector<uint8_t>& data, int32_t data_len){
        on_inc_tcp_master_data(data, data_len);
    });

    _tcp.init(_my_node->tcp_port);

    _udp.init(_my_node->udp_port);

    _udp.set_on_client_data_callback([&](Net_client* client, const std::vector<uint8_t>& data, int32_t data_len){
        on_inc_client_udp_data(client, data, data_len);
    });

    _tcp.set_on_client_data_callback([&](Net_client* client, const std::vector<uint8_t>& data, int32_t data_len) {
        on_inc_client_tcp_data(client, data, data_len);
    });

    _tcp.set_on_client_connect_callback([&](Net_client* client) {
        on_client_connect(client);
    });

    _tcp.set_on_client_disconnect_callback([&](Net_client* client) {
        on_client_disconnect(client);
    });
}

Net_slave::~Net_slave() {}

/// <summary>
/// When we get data from the master socket
/// </summary>
/// <param name="data"></param>
/// <param name="data_len"></param>
void Net_slave::on_inc_tcp_master_data(const std::vector<uint8_t>& data, int32_t data_len) {
    if (data_len > 0) {
        MsgType type = (MsgType)data[0];

        switch (type) {
            case MsgType::NetFromMasterToSlaveCommand:
            {
                Net_master_to_slave_command cmd(data, 0);
                handle_master_command(cmd);
                break;
            }
            default:
                TRACE("[NET-SLAVE][UPDATE][ERROR][Unknown message type: %d]\n", type);
                break;
            }
    }
}

/// <summary>
/// When we get incoming UDP data from a player
/// </summary>
/// <param name="client"></param>
/// <param name="data"></param>
/// <param name="data_len"></param>
void Net_slave::on_inc_client_udp_data(Net_client* client, const std::vector<uint8_t>& data, int32_t data_len) {
    // handle incoming UDP data from players
    int32_t len = data_len;
    uint32_t off = 0;

    while (len > 0) {
        MsgType type = (MsgType)(uint8_t)data[off];

        switch (type) {
        
            case MsgType::NetPlayerPos:
            {
                Net_pos pos(data, off);

                off += sizeof(Net_pos);
                len -= sizeof(Net_pos);

                // we got an updated player position
                // so we have to update the Net_session_player position values

                _session_id_lookup[client->info.session_id]->on_inc_pos(pos);

                break;
            }
            // we want this via UDP so we get a more accurate timestamp
            case MsgType::NetPlayerSyncTimeRequest:
            {
                Net_player_sync_time_request req(data, off);
                off += sizeof(Net_player_sync_time_request);
                len -= sizeof(Net_player_sync_time_request);

                Net_player_sync_time_response res;
                res.t0 = req.t0;
                res.t1 = std::chrono::high_resolution_clock::now().time_since_epoch().count();

                res.session_time = _session_id_lookup[client->info.session_id]->get_time();

                client->add_tcp_data(&res, sizeof(Net_player_sync_time_response));

                break;
            }
            case MsgType::NetPlayerChat:
            {
                //Net_chat()
                break;
            }
            default: return;
        }
    }
}

bool Net_slave::find_client(Net_client* client) const {
    for (int i = 0; i < _sessions.size(); ++i) {
        Net_client* c = _sessions[i]->find_client(client);

        if (c != NULL) {
            return true;
        }
    }

    return false;
}


Net_session* Net_slave::find_empty_session() const {
    for (int i = 0; i < _sessions.size(); ++i) {
        if (_sessions[i]->is_empty()) {
            return _sessions[i].get();
        }
    }

    return NULL;
}

void Net_slave::set_session_private(Net_session* session, bool priv) {
    if (session->is_private() == priv) {
        return;
    }

    if (priv) {
        // move from public to private
        for (int i = 0; i < _public_sessions.size(); ++i) {
            if (_public_sessions[i]->get_id() == session->get_id()) {
                _public_sessions.erase(_public_sessions.begin() + i);
                break;
            }
        }

        _private_sessions.push_back(session);
    }
    else {
        // move from private to public
        for (int i = 0; i < _private_sessions.size(); ++i) {
            if (_private_sessions[i]->get_id() == session->get_id()) {
                _private_sessions.erase(_private_sessions.begin() + i);
                break;
            }
        }

        _public_sessions.push_back(session);
    }

    session->set_private(priv);
}

Net_session* Net_slave::find_client_session(Net_client* client) const {
    for (int i = 0; i < _sessions.size(); ++i) {
        Net_client* c = _sessions[i]->find_client(client);

        if (c != NULL) {
            return _sessions[i].get();
        }
    }

    return NULL;
}

void Net_slave::reset_session_lookups(Net_session* session) {
    if (_session_code_lookup.find(session->session_code_hash) != _session_code_lookup.end()) {
        _session_code_lookup.erase(session->session_code_hash);
    }

    if (_session_id_lookup.find(session->get_id()) != _session_id_lookup.end()) {
        _session_id_lookup.erase(session->get_id());
    }
}

void Net_slave::set_session_lookups(Net_session* session) {
    _session_code_lookup[session->session_code_hash] = session;
    _session_id_lookup[session->get_id()] = session;
}


/// <summary>
/// Send the session data to the master so it knows the current status
/// </summary>
/// <param name="session"></param>
void Net_slave::sync_session_with_master(Net_session* session) {
    Net_from_slave_sync_session sync;

    memcpy(sync.code, session->session_code, 8);
    sync.num_players = session->get_num_players();
    sync.session_id = session->get_id();

    _master_connection.add_data(&sync, sizeof(Net_from_slave_sync_session));
}

void Net_slave::on_inc_client_tcp_data(Net_client* client, const std::vector<uint8_t>& data, int32_t data_len) {
    // incoming TCP data from clients/players

    MsgType type = MsgType::None;
    uint32_t off = 0;
    int32_t len = data_len;
    
    if (client->info.type == NetClientType::Unauthenticated) {
        // we only allow authentication packets
        MsgType type = (MsgType)((uint8_t)data[off]);

        switch (type) {
            case MsgType::NetAuthenticatePlayer:
            {
                Net_authenticate_player auth(data, off);
                len -= sizeof(Net_authenticate_player);
                off += sizeof(Net_authenticate_player);

                if (auth.client_password == _my_node->client_password) {
                    TRACE("[NET-SLAVE][ON-INC-TCP-DATA][NetAuthenticatePlayer][SUCCESS]\n");
                    client->info.type = NetClientType::Player;

                    Net_success succ(NetSuccessType::None);

                    client->add_tcp_data(&succ, sizeof(Net_success));
                    break;
                }
                else {
                    TRACE("[NET-SLAVE][ON-INC-TCP-DATA][NetAuthenticatePlayer][FAIL][Invalid password]\n");

                    Net_error err(NetErrorType::InvalidPassword);
                    
                    client->add_tcp_data(&err, sizeof(Net_error));

                    _tcp.disconnect(client);
                    return;
                }
                break;
            }
            default:
                // disconnect
                TRACE("[NET-SLAVE][ON-INC-TCP-DATA][ERROR][Client tried to send message without authenticated state]\n");
                _tcp.disconnect(client);
                
                return;
        }
    }

    while (len > 0) {
        type = (MsgType)((uint8_t)data[off]);

        switch (type) {
            case MsgType::NetPlayerSlaveListSessionsRequest:
            {
                // player wants a list of sessions
                off += sizeof(Net_player_slave_list_sessions_request);
                len -= sizeof(Net_player_slave_list_sessions_request);

                Net_player_slave_list_sessions_response resp;
                resp.num_sessions = 0;
                
                for (auto sess : _public_sessions) {
                    if (sess->is_full() || sess->is_empty()) {
                        continue;
                    }

                    resp.sessions.push_back(Net_slave_session_item(sess->get_id(), "test", sess->get_num_players(), sess->get_max_players()));
                    resp.num_sessions++;
                }

                client->add_tcp_data(&resp, resp.header_size());

                // if theres no sessions, then we send an error?
                if (resp.num_sessions > 0) {
                    client->add_tcp_data(&resp.sessions[0], resp.data_size());
                }

                break;
            }
            case MsgType::NetPlayerHostSessionRequest:
            {
                Net_player_host_session_request req(data, off);
                off += sizeof(Net_player_host_session_request);
                len -= sizeof(Net_player_host_session_request);

                Net_session* session = find_client_session(client);

                // dont allow a player to create a session if there already is a session associated with that player
                if (session != NULL) {
                    TRACE("[NET-SLAVE][NetPlayerHostSessionRequest][ERROR][Already have sesssion]\n");
                    Net_error reject(NetErrorType::AlreadyHaveSession);

                    client->add_tcp_data(&reject, sizeof(Net_error));
                    continue;
                }

                // make sure there is an empty session
                session = find_empty_session();

                if (session == NULL) {
                    TRACE("[NET-SLAVE][NetPlayerHostSessionRequest][ERROR][No available sessions]\n");
                    Net_error reject(NetErrorType::NoAvailableSessions);

                    client->add_tcp_data(&reject, sizeof(Net_error));
                    continue;
                }

                // remove old session lookups
                reset_session_lookups(session);
                // generate a new private code
                session->generate_session_code();
                // assign the session as public/private
                set_session_private(session, req.is_private == 1);
                // add the new session lookups
                set_session_lookups(session);

                session->add_player_and_broadcast(client, false, true);

                Net_player_host_session_response resp;
                memcpy(resp.code, session->session_code, 8);
                resp.session_id = session->get_id();
                resp.max_players = session->get_max_players();
                
                client->add_tcp_data(&resp, sizeof(Net_player_host_session_response));

                // lastly, notify the master that we have a user in the session
                sync_session_with_master(session);

                print_sessions_summary();

                break;
            }
            case MsgType::NetPlayerSlaveJoinPublicSessionRequest:
            {
                Net_player_slave_join_public_session_request sess(data, off);
                off += sizeof(Net_player_slave_join_public_session_request);
                len -= sizeof(Net_player_slave_join_public_session_request);

                if (_session_id_lookup.find(sess.session_id) == _session_id_lookup.end()) {
                    TRACE("[NET-SLAVE][NetPlayerSlaveJoinPublicSessionRequest][ERROR][Session not found]\n");
                    // session doesnt exist
                    Net_error err(NetErrorType::SessionNotFound);
                    client->add_tcp_data(&err, sizeof(Net_error));
                    continue;
                }

                Net_session* session = _session_id_lookup[sess.session_id];

                if (session->is_full()) {
                    TRACE("[NET-SLAVE][NetPlayerSlaveJoinPublicSessionRequest][ERROR][Session is full]\n");

                    Net_error err(NetErrorType::SessionIsFull);
                    client->add_tcp_data(&err, sizeof(Net_error));
                    continue;
                }

                Net_player_slave_join_public_session_response resp;
                resp.session_id = session->get_id();

                client->add_tcp_data(&resp, sizeof(Net_player_slave_join_public_session_response));

                // will assign session_id on client
                session->add_player_and_broadcast(client, true, false);

                // also send out the game config to the player that joined
                client->add_tcp_data(&_session_id_lookup[client->info.session_id]->game_config, _session_id_lookup[client->info.session_id]->game_config.size());

                sync_session_with_master(session);

                print_sessions_summary();

                break;
            }
            case MsgType::NetPlayerSlaveJoinPrivateSessionRequest:
            {
                // when a player tries to join a session
                Net_player_slave_join_private_session_request sess(data, off);
                off += sizeof(Net_player_slave_join_private_session_request);
                len -= sizeof(Net_player_slave_join_private_session_request);

                mmh::Hash_key key(sess.code);

                if (_session_code_lookup.find(key.hash) == _session_code_lookup.end()) {
                    TRACE("[NET-SLAVE][NetPlayerSlaveJoinPrivateSessionRequest][ERROR][Session not found]\n");

                    // session doesnt exist
                    Net_error err(NetErrorType::SessionNotFound);
                    client->add_tcp_data(&err, sizeof(Net_error));
                    continue;
                }

                // we found the session
                Net_session* session = _session_code_lookup[key.hash];
                    
                if (session->is_full()) {
                    TRACE("[NET-SLAVE][NetPlayerSlaveJoinPrivateSessionRequest][ERROR][Session is full]\n");

                    Net_error err(NetErrorType::SessionIsFull);

                    client->add_tcp_data(&err, sizeof(Net_error));
                    continue;
                }

                Net_player_slave_join_private_session_response resp;
                resp.session_id = session->get_id();
                
                client->add_tcp_data(&resp, sizeof(Net_player_slave_join_private_session_response));

                // send out Net_player_has_joined_session to all players in session
                session->add_player_and_broadcast(client, true, false);

                // also send out the game config to the player that joined
                client->add_tcp_data(&_session_id_lookup[client->info.session_id]->game_config, _session_id_lookup[client->info.session_id]->game_config.size());

                sync_session_with_master(session);

                print_sessions_summary();
                
                break;
            }
            case MsgType::NetPlayerLeaveSessionRequest:
            {
                // when a player wants to leave a session
                Net_player_leave_session_request req(data, off);
                off += sizeof(Net_player_leave_session_request);
                len -= sizeof(Net_player_leave_session_request);

                if (_session_id_lookup.find(req.session_id) == _session_id_lookup.end()) {
                    TRACE("[NET-SLAVE][NetPlayerLeaveSessionRequest][ERROR][Session not found]\n");

                    Net_error err(NetErrorType::SessionNotFound);
                    client->add_tcp_data(&err, sizeof(Net_error));
                    
                    break;
                }

                Net_session* session = _session_id_lookup[req.session_id];
                session->remove_player_and_broadcast(client, true);
                
                client->reset_session();

                Net_success success(NetSuccessType::None);
                client->add_tcp_data(&success, sizeof(Net_success));

                sync_session_with_master(session);

                print_sessions_summary();
                break;
            }
            case MsgType::NetPlayerSetGameRuleInt:
            {
                // When a gamerule is being set by the owner
                Net_player_set_gamerule_int gamerule(data, off);
                off += sizeof(Net_player_set_gamerule_int);
                len -= sizeof(Net_player_set_gamerule_int);

                Net_game_rule_updated rule;
                rule.rule_id = gamerule.rule_id;
                rule.rule_value = gamerule.rule_value;

                _session_id_lookup[client->info.session_id]->set_game_rule(rule.rule_id, rule.rule_value);

                // broadcasts the rule change to all players
                _session_id_lookup[client->info.session_id]->broadcast_tcp(&rule, sizeof(Net_game_rule_updated));

                break;
            }
            case MsgType::NetPlayerStartGameSessionRequest:
            {
                Net_player_start_game_session_request req(data, off);
                off += sizeof(Net_player_start_game_session_request);
                len -= sizeof(Net_player_start_game_session_request);
                
                Net_player_start_game_session_response res;
                res.ok = 1;
                res.start_timestamp = -5;


                if (_session_id_lookup[client->info.session_id]->can_game_be_started()) {
                    _session_id_lookup[client->info.session_id]->start_game_in_seconds(5);

                    Net_game_session_will_start start;
                    start.ok = 1;
                    start.start_timestamp = -5;

                    // broadcast game start event to all players
                    _session_id_lookup[client->info.session_id]->broadcast_tcp(&start, sizeof(Net_game_session_will_start));
                }
                else {
                    res.ok = 0;
                }

                client->add_tcp_data(&res, sizeof(Net_player_start_game_session_response));

                break;
            }
            default:
                TRACE("[NET-SLAVE][Uknown message][%d]\n", type);
                break;
        }
    }
}

bool Net_slave::init() {
    // connect to master
    connect_to_master();

    return true;
}

bool Net_slave::validate_ini() {
    if (_my_node->udp_port == 0) {
        TRACE("ERROR: ini must include udp_port\n");
        return false;
    }

    if (_my_node->tcp_port == 0) {
        TRACE("ERROR: ini must include tcp_port\n");
        return false;
    }

    return true;
}

/// <summary>
/// Create our sessions during startup, If all sessions are marked as running
/// then we dont allow player host requests on this node.
/// Its really important that the master node keeps a correct track of the
/// amount of sessions available on the node
/// </summary>
void Net_slave::setup_sessions() {

    for (int i = 0; i < _my_node->max_sessions; ++i) {
        auto sess = std::make_unique<Net_session>(
            _my_node->max_users_per_session,
            _my_node->keepalive_time_seconds,
            &_tcp,
            &_udp);
        _sessions.push_back(std::move(sess));
    }
}

void Net_slave::print_sessions() const {
    TRACE("--- Num sessions: %d\n", _sessions.size());
    for (int i = 0; i < _sessions.size(); ++i) {
        TRACE("   Session: %d, max: %d, num: %d, empty: %s\n", _sessions[i]->get_id(), _sessions[i]->get_max_players(), _sessions[i]->get_num_players(),
            (_sessions[i]->is_empty() ? "YES" : "NO")
        );
    }
}

void Net_slave::print_sessions_summary() const {
    TRACE("--- Session summary \n   | Num sessions: %d\n", _sessions.size());

    int num_empty = 0;
    int num_non_empty = 0;

    for (int i = 0; i < _sessions.size(); ++i) {
        if (_sessions[i]->is_empty()) {
            num_empty++;
        }
        else {
            num_non_empty++;
        }
    }

    TRACE("   | Num Empty sessions: %d\n", num_empty);
    TRACE("   | Num Non-Empty sessions: %d\n", num_non_empty);
}

/// <summary>
/// When a player has connected to this slave
/// </summary>
/// <param name="client"></param>
void Net_slave::on_client_connect(Net_client* client) {
    _num_players++;

    // create a new net session player
    auto player = std::make_shared<Net_session_player>();
    player->net_client_id = client->info.client_id;
    _client_id_lookup[client->info.client_id] = player;

    _udp.establish_client_connection(client);

    // send a request to the client to connect to our UDP port
    Net_Udp_client_connection_info udpconn;
    udpconn.client_id = client->info.client_id;
    memset(udpconn.ip, 0, 64);
    strcpy(udpconn.ip, _my_node->ip.c_str());
    udpconn.port = _my_node->udp_port;
    
    client->add_tcp_data((void*)&udpconn, sizeof(Net_Udp_client_connection_info));
}

/// <summary>
/// When a player has disconnected (either controlled or uncontrolled)
/// </summary>
/// <param name="client"></param>
void Net_slave::on_client_disconnect(Net_client* client) {
    _num_players--;

    TRACE("[NET-SLAVE][CLIENT DISCONNECT]\n");
    // when a client has disconnected, we need to make sure we delete all references to the client object

    // delete the client
    if (client->info.client_id != 0 && _client_id_lookup.find(client->info.client_id) != _client_id_lookup.end()) {
        _client_id_lookup.erase(client->info.client_id);
    }

    // remove player from the session
    if (client->info.session_id != 0 && _session_id_lookup.find(client->info.session_id) != _session_id_lookup.end()) {
        Net_session* session = _session_id_lookup[client->info.session_id];
        session->disconnect(client->info.client_id);

        sync_session_with_master(session);
    }

    // remove the player from the UDP server lookup tables
    _udp.remove_client(client);
}

/// <summary>
/// When a command has been received from the master node
/// Make sure we reply back the data the master requests
/// </summary>
/// <param name="command"></param>
void Net_slave::handle_master_command(const Net_master_to_slave_command& command) {
    switch ((NetMasterToSlaveCommand)command.command) {
        case NetMasterToSlaveCommand::ReportHealth:
        {
            TRACE("[NET-SLAVE][HANDLE-MASTER-COMMAND][Will send health report]\n");

            Process_stats_snapshot snapshot;
            _stats->gather_stats(snapshot);

            float a = (float)snapshot.num_good_ticks;
            if (a == 0 && snapshot.num_lag_ticks == 0) {
                a = 1.0f;
            }

            float b = (float)snapshot.ram_virt_total_bytes;
            if (b == 0) {
                b = 1.0f;
            }

            Net_slave_health_snapshot msg(
                (float)snapshot.ram_virt_process_used_bytes / b,
                (float)snapshot.num_lag_ticks / ((float)(snapshot.num_lag_ticks + a)),
                snapshot.avg_tick_idle_time, 
                0,
                _num_players); // cpu load isnt working

            msg.set_buffer(_data_buffer, 0);
            msg.print();

            _master_connection.add_data(&msg, sizeof(Net_slave_health_snapshot));

            break;
        }
        default:
            TRACE("[NET-SLAVE][HANDLE-MASTER-COMMAND][ERROR][Unknown command: %d]\n", command.command);
            break;
    }
}

void Net_slave::print_stats() {
    Process_stats_snapshot snapshot;

    _stats->gather_stats(snapshot);

    TRACE(" ---- STATS REPORT -----\n");
    TRACE("Num good ticks: %lu\nNum lag ticks: %lu\nAverage tick idle time: %ld\nCPU Load process: %f\nCPU Load global: %f\n", 
        snapshot.num_good_ticks, 
        snapshot.num_lag_ticks, 
        snapshot.avg_tick_idle_time, 
        (float)snapshot.cpu_load_process, 
        (float)snapshot.cpu_load_total);
    TRACE("--- RAM: \n");
    TRACE("Physical total: %lu KB\nPhysical process used: %lu KB\nPhysical used: %lu KB\nVirtual Process used: %lu KB\nVirtual Total: %lu KB\nVirtual used: %lu KB\n", 
        ((snapshot.ram_phys_total_bytes / 1000)), 
        ((snapshot.ram_phys_process_used_bytes / 1000)),
        ((snapshot.ram_phys_used_bytes / 1000)),
        ((snapshot.ram_virt_process_used_bytes / 1000)),
        ((snapshot.ram_virt_total_bytes / 1000)),
        ((snapshot.ram_virt_used_bytes / 1000)));
}

/// <summary>
/// Initiate the connection to our master. We also send our authentication so
/// the master can mark this connection as a slave connection
/// </summary>
/// <returns></returns>
bool Net_slave::connect_to_master() {
    TRACE("Slave IP: %s\n", _my_node->ip.c_str());

    auto master = _ini_file->get_master();

    if (!_master_connection.init(master->ip.c_str(), master->hostname.c_str(), master->is_ip_set, master->tcp_port)) {
        TRACE("[NET-SLAVE][INIT][FATAL ERROR][Unable to init connection to master]\n");
        return false;
    }

    // the first thing we need to send is a authentication packet 
    // that authenticates with the master
    Net_authenticate_slave reg(_my_node->id, _master.node->master_password);
    reg.set_buffer(_data_buffer, 0);

    _master_connection.add_data(&reg, sizeof(Net_authenticate_slave));
    
    TRACE("[NET-SLAVE][INIT][Authentication packet sent]\n");

    // send our configuration
    Net_slave_config config;
    memset(config.hostname, 0, 64);
    memset(config.ip, 0, 64);

    strcpy(config.hostname, _my_node->hostname.c_str());
    strcpy(config.ip, _my_node->ip.c_str());

    config.tcp_port = _my_node->tcp_port;
    config.udp_port = _my_node->udp_port;
    config.node_id = _my_node->id;
    
    _master_connection.add_data(&config, sizeof(Net_slave_config));
    
    /*
    
    config.set_buffer(_data_buffer, sizeof(Net_slave_config));

    if (!_master_connection.send_data(_data_buffer, sizeof(Net_slave_config))) {
        TRACE("[NET-SLAVE][INIT][ERROR][Unable to send slave config]\n");
        return false;
    }
    
    */

    return true;
}

/// <summary>
/// Called once per tick
/// + Read from master TCP socket
/// + Read from client TCP socket (use reliable UDP instead?)
/// + Read from client UDP socket
/// </summary>
void Net_slave::update() {
    // make sure we have a connection active to the master server
    if (!_master_connection.is_initialized()) {
        if (!connect_to_master()) {
            return;
        }
    }

    auto now = std::chrono::high_resolution_clock::now();

    double delta = (double)(std::chrono::duration_cast<std::chrono::milliseconds>(now - _last_tick).count() / 1000.0);

    // read all player TCP data
    _tcp.read();

    // currently we only update the read for UDP, sending data is done in other places
    // should probably redesign this pattern
    _udp.read();

    // send out all queued up TCP data to players (should be very few packets)
    _tcp.send_client_data();
    
    for (int i = 0; i < _sessions.size(); ++i) {
        _sessions[i]->update_game(delta, now);
        _sessions[i]->send_udp(now);
    }

    // read/send data from the master socket
    _master_connection.update();


    if (_next_slave_report < now) {
        //print_stats();
        _next_slave_report = now + std::chrono::seconds(5);
    }
}