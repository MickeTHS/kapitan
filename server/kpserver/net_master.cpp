#include "net_master.h"

#include "tcp_server.h"
#include "net_packet.h"
#include "ini_file.h"
#include "trace.h"

#include <algorithm>
#include <ctime>

Net_master::Net_master(Ini_file* file) 
    :   _ini_file(file) {

    _data_buffer.resize(1000);
    _my_node = _ini_file->get_me();
    
    _next_slave_report = std::chrono::high_resolution_clock::now() - std::chrono::minutes(60);
    
    //std::vector<std::shared_ptr<Ini_node>> slaves;
    //file.get_slaves(slaves);

    _tcp.init(_my_node->tcp_port);

    _tcp.set_on_client_data_callback([&](Net_client* client, const std::vector<uint8_t>& data, int32_t len) {
        on_inc_tcp_data(client, data, len);
    });

    _tcp.set_on_client_connect_callback([&](Net_client* client) {
        on_client_connect(client);
    });

    _tcp.set_on_client_disconnect_callback([&](Net_client* client) {
        on_client_disconnect(client);
    });
}

Net_master::~Net_master() {

}

void Net_master::on_client_connect(Net_client* client) {
    //TRACE("[NET-MASTER][New slave connected]\n");
    // at this moment we dont know if its a slave or a client
    // wait for authentication
    
    client->info.type = NetClientType::Unauthenticated;

    // we must receive a password or we will disconnect the client
}

void Net_master::on_client_disconnect(Net_client* client) {
    // when a client has disconnected, we need to make sure we delete all references to the client object

    if (client->info.type == NetClientType::SlaveNode) {
        remove_slave(client);
    }
    else if (client->info.type == NetClientType::Player) {
        remove_player(client);
    }

}

void Net_master::remove_slave(Net_client* client) {
    // delete the client
    if (client->info.client_id != 0 && _client_id_lookup.find(client->info.client_id) != _client_id_lookup.end()) {
        _client_id_lookup.erase(client->info.client_id);
    }

    Net_slave_info* slave = NULL;

    // remove from lookups
    if (_slave_by_client_id.find(client->info.client_id) == _slave_by_client_id.end()) {
        slave = _slave_by_client_id[client->info.client_id];
    }
    else {
        return;
    }

    _slave_by_client_id.erase(slave->client->info.client_id);
    _slave_by_slave_id.erase(slave->slave_id);

    for (int i = 0; i < _slaves_health.size(); ++i) {
        if (_slaves_health[i].get() == slave) {
            _slaves_health.erase(_slaves_health.begin() + i);
            break;
        }
    }
}

/// <summary>
/// Do we need to keep track of this on the master?
/// Probably not
/// Why would we need to keep track of net clients on sessions?
/// The master will only have temporary Net_client connections, so this makes no sense
/// </summary>
/// <param name="client"></param>
void Net_master::remove_player(Net_client* client) {

    // delete the client
    if (client->info.client_id != 0 && _client_id_lookup.find(client->info.client_id) != _client_id_lookup.end()) {
        _client_id_lookup.erase(client->info.client_id);
    }

    // remove player from the session
    if (client->info.session_id != 0 && _session_id_lookup.find(client->info.session_id) != _session_id_lookup.end()) {
        Net_slave_info* info = _session_id_lookup[client->info.session_id];
        info->num_players--;
    }
}

Net_slave_info* Net_master::get_slave(Net_client* client) {
    if (_slave_by_client_id.find(client->info.client_id) == _slave_by_client_id.end()) {
        return nullptr;
    }

    return _slave_by_client_id[client->info.client_id];
}

/// <summary>
/// When the client has authenticated correctly, we add it to our lookups so we can keep track of them
/// </summary>
/// <param name="client"></param>
/// <param name="auth"></param>
void Net_master::add_authenticated_slave(Net_client* client, const Net_authenticate_slave& auth) {
    client->info.type = NetClientType::SlaveNode;

    if (_slave_by_slave_id.find(auth.slave_id) != _slave_by_slave_id.end()) {
        TRACE("[NET-MASTER][NOTICE][Authenticated slave is already registered]\n");
        return;
    }

    // link up the client to a slave_node_info
    auto slave = std::make_unique<Net_slave_info>(client, auth.slave_id, 0, 0);

    _slave_by_slave_id[auth.slave_id] = slave.get();
    _slave_by_client_id[client->info.client_id] = slave.get();

    _slaves_health.push_back(std::move(slave)); // keep it last because we dont know the health yet
    
    TRACE("[NET-MASTER][New slave node registered successfully]\n");
}

/// <summary>
/// When we get TCP data it can be either from players or from slaves
/// client->info.type determines who is what
/// if the type is NetClientType::Unauthenticated, we dont know and we
/// wait for authentication
/// slaves authenticates using a master password
/// players authenticate using a client password
/// </summary>
/// <param name="client"></param>
/// <param name="data"></param>
/// <param name="data_len"></param>
void Net_master::on_inc_tcp_data(Net_client* client, const std::vector<uint8_t>& data, int32_t data_len) {
    
    uint32_t pos = 0;
    int32_t len = data_len;

    while (data_len > 0) {
        MsgType type = (MsgType)((uint8_t)data[pos]);

        if (client->info.type == NetClientType::Unauthenticated) {
            // we only allow authentication packets if the client is unauthenticated
            // if the client isnt able to authenticate, we disconnect it directly
            switch (type) {
            case MsgType::NetAuthenticatePlayer:
            {
                Net_authenticate_player auth(data, pos);

                if (auth.client_password == _my_node->client_password) {
                    TRACE("[NET-MASTER][ON-INC-TCP-DATA][NetAuthenticatePlayer][SUCCESS]\n");
                    client->info.type = NetClientType::Player;

                    pos += sizeof(Net_authenticate_player);
                    len -= sizeof(Net_authenticate_player);
                }
                else {
                    TRACE("[NET-MASTER][ON-INC-TCP-DATA][NetAuthenticatePlayer][FAIL][Invalid password]\n");
                    _tcp.disconnect(client);
                    return;
                }
                break;
            }
            case MsgType::NetAuthenticateSlave:
            {
                Net_authenticate_slave auth(data, pos);

                if (auth.master_password == _my_node->master_password) {
                    TRACE("[NET-MASTER][ON-INC-TCP-DATA][NetAuthenticateSlave][SUCCESS]\n");

                    add_authenticated_slave(client, auth);
                }
                else {
                    TRACE("[NET-MASTER][ON-INC-TCP-DATA][NetAuthenticateSlave][FAIL][Invalid master password]\n");
                    _tcp.disconnect(client);
                    return;
                }

                pos += sizeof(Net_authenticate_slave);
                len -= sizeof(Net_authenticate_slave);
            }
            default:
                // disconnect
                break;
            }

            TRACE("[NET-MASTER][ON-INC-TCP-DATA][ERROR][Client tried to send message without authenticated state]\n");
            _tcp.disconnect(client);

            return;
        }

        switch (type) {

            // ---------- PLAYER MESSAGES ------- //
            case MsgType::NetPlayerSlaveNodeRequest: 
            {
                TRACE("[NET-MASTER][NetPlayerSlaveNodeRequest]\n");

                // no need to populate packet, its only a uint8
                // just send a slave config
                // But first we need to check that the player has been registered on the master node


                //Net_player_slave_node_request req;

                pos += sizeof(Net_player_slave_node_request);
                len -= sizeof(Net_player_slave_node_request);

                Net_slave_info* slave = _slaves_health[0].get();

                Net_player_slave_node_response resp;
                strcpy(resp.ip, slave->ip);
                resp.slave_id = slave->slave_id;
                resp.tcp_port = slave->tcp_port;
                
                client->add_tcp_data(&resp, sizeof(Net_player_slave_node_response));

                break;
            }
            case MsgType::NetPlayerJoinSessionRequest:
            {
                TRACE("[NET-MASTER][NetPlayerJoinSessionRequest]\n");
                // when a session join request comes in to the master, it means
                // its a private session that isnt listed

                // We need to send a reply with the node config of the session
                // When the client gets the response that the session was
                // found at the provided node, it needs to connect to that noce
                Net_player_join_session_request req(data, pos);

                pos += sizeof(Net_player_join_session_request);
                len -= sizeof(Net_player_join_session_request);

                mmh::Hash_key key(req.code);

                // Lookup the session code to find the slave node that the session is played on
                if (_session_code_lookup.find(key.hash) != _session_code_lookup.end()) {
                    TRACE("[NET-MASTER][NetPlayerJoinSessionRequest][OK]\n");

                    // we found the session
                    Net_slave_info* slave = _session_code_lookup[key.hash];
                    
                    Net_player_slave_node_response resp;
                    strcpy(resp.ip, slave->ip);
                    resp.slave_id = slave->slave_id;
                    resp.tcp_port = slave->tcp_port;

                    // the client now has all the information needed to connect to the slave node
                    client->add_tcp_data(&resp, sizeof(Net_player_slave_node_response));
                }
                else {
                    TRACE("[NET-MASTER][ERROR][Session not found]\n");
                    Net_error error(NetErrorType::SessionNotFound);
                
                    // we didnt find the session so send an error instead
                    client->add_tcp_data(&error, sizeof(Net_error));
                }
                
                break;
            }

            // ---------- SLAVE MESSAGES ------- //
            case MsgType::NetFromSlaveSyncSession: 
            {
                // Keepalive messages are sent from the slave node to the master to indicate that everything is fine with the session
                TRACE("[NET-MASTER][NetFromSlaveSyncSession]\n");

                Net_from_slave_sync_session session(data, pos);

                Net_slave_info* slave = get_slave(client);

                pos += sizeof(Net_from_slave_sync_session);
                len -= sizeof(Net_from_slave_sync_session);

                slave->set_session(session.session_id, session.code, session.num_players);

                break;
            }
            case MsgType::NetSlaveHealthReport:
            {
                TRACE("[NET-MASTER][NetSlaveHealthReport][Got health report from slave]\n");
                auto slave = get_slave(client);

                if (slave == nullptr) {
                    TRACE("[NET-MASTER][NetSlaveHealthReport][ERROR][Cant find slave with client_id: %d]\n", client->info.client_id);
                    return;
                }

                Net_slave_health_snapshot health(data, pos);
                TRACE("Health report from slave_id: %d\n", slave->slave_id);

                slave->set_health_rating(health);
                std::sort(_slaves_health.begin(), _slaves_health.end(), best_health_is_first());

                health.print();

                pos += sizeof(Net_slave_health_snapshot);
                len -= sizeof(Net_slave_health_snapshot);

                break;
            }
            default:
                TRACE("[NET-MASTER][ON_INC_DATA][ERROR][Unknown message type: %d]\n", type);

                pos = 0;
                len = 0;
                break;
        }
    }
}

void Net_master::update() {
    auto now = std::chrono::high_resolution_clock::now();

    // perform slave report ?
    if (_next_slave_report < now) {
        // set the next time we want a report
        _next_slave_report = now + std::chrono::seconds(_my_node->slave_sync_interval_seconds);

        // slaves will connect to us as net_clients
        Net_master_to_slave_command report(NetMasterToSlaveCommand::ReportHealth);

        report.set_buffer(_data_buffer, 0);

        if (!_tcp.send_data_to_all(_data_buffer, sizeof(NetMasterToSlaveCommand))) {
            TRACE("[NET-MASTER][UPDATE][Demand report send failed]\n");
        }
        else {
            TRACE("[NET-MASTER][UPDATE][Demand report sent]\n");
        }
    }
}

Net_slave_info::Net_slave_info(Net_client* client_, uint32_t slave_id_, uint32_t session_id_start_range_, uint32_t num_sessions_)
    :   client(client_), 
        slave_id(slave_id_), 
        health_rating(-1), 
        session_id_start_range(session_id_start_range_),
        num_sessions(num_sessions_),
        num_players(0),
        tcp_port(0) {
    memset(ip, 0, 64);
}

void Net_slave_info::set_health_rating(const Net_slave_health_snapshot& snap) {
    // calculate a health rating used to determine the healthiest node
    health_rating = snap.avg_tick_idle_time * ((1.0f - snap.pct_good_vs_lag_ticks) * 100);
    num_players = snap.num_connected_players;
}

Net_session_info* Net_slave_info::get_session(uint32_t session_id) {
    if (_session_id_lookup.find(session_id) == _session_id_lookup.end()) {
        return NULL;
    }

    return _session_id_lookup[session_id];
}

/// <summary>
/// This is called during startup
/// Populates all sessions that can exist on a slave
/// </summary>
/// <param name="id"></param>
/// <param name="code"></param>
void Net_slave_info::add_session(uint32_t id, const char* code) {
    auto session = std::make_unique<Net_session_info>();
    strcpy(session->code, code);
    session->id = id;
    session->set_session_hash();

    _session_id_lookup[session->id] = session.get();
    _session_code_lookup[session->session_code_hash] = session.get();

    _sessions.push_back(std::move(session));
}

/// <summary>
/// Updates the session code
/// </summary>
/// <param name="id"></param>
/// <param name="code"></param>
void Net_slave_info::set_session(uint32_t id, const char* code, uint32_t num_players) {
    if (_session_id_lookup.find(id) == _session_id_lookup.end()) {
        return;
    }

    auto session = _session_id_lookup[id];

    // remove the old hash lookup
    if (_session_code_lookup.find(session->session_code_hash) != _session_code_lookup.end()) {
        _session_code_lookup.erase(session->session_code_hash);
    }
    
    strcpy(session->code, code);
    session->set_session_hash();
    session->num_players = num_players;
    _session_code_lookup[session->session_code_hash] = session;
}

/*void Net_slave_info::keepalive() {
    _last_keepalive = std::chrono::high_resolution_clock::now();
}*/
