#include "net_master.h"

#include "tcp_server.h"
#include "net_packet.h"

#include <algorithm>
#include <ctime>

Net_master::Net_master(Tcp_server* tcp, const Ini_file& file) : _ini_file(file) {
    _tcp = tcp;
    _data_buffer.resize(1000);
    _my_node = _ini_file.get_me();
    
    _next_slave_report = std::chrono::high_resolution_clock::now() - std::chrono::minutes(60);
    
    //std::vector<std::shared_ptr<Ini_node>> slaves;
    //file.get_slaves(slaves);

    _tcp->set_on_client_data_callback([&](Net_client* client, const std::vector<uint8_t>& data, int32_t len) {
        on_inc_tcp_data(client, data, len);
    });

    _tcp->set_on_client_connect_callback([&](Net_client* client) {
        on_client_connect(client);
    });

    _tcp->set_on_client_disconnect_callback([&](Net_client* client) {
        on_client_disconnect(client);
    });
}

Net_master::~Net_master() {

}

void Net_master::on_client_connect(Net_client* client) {
    //printf("[NET-MASTER][New slave connected]\n");
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

    std::shared_ptr<Net_slave_info> slave = nullptr;

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
        if (_slaves_health[i] == slave) {
            _slaves_health.erase(_slaves_health.begin() + i);
            break;
        }
    }
}

void Net_master::remove_player(Net_client* client) {

    // delete the client
    if (client->info.client_id != 0 && _client_id_lookup.find(client->info.client_id) != _client_id_lookup.end()) {
        _client_id_lookup.erase(client->info.client_id);
    }

    // remove player from the session
    if (client->info.session_id != 0 && _session_id_lookup.find(client->info.session_id) != _session_id_lookup.end()) {
        _session_id_lookup[client->info.session_id]->disconnect(client->info.client_id);
    }
}

std::shared_ptr<Net_slave_info> Net_master::get_slave(Net_client* client) {
    if (_slave_by_client_id.find(client->info.client_id) == _slave_by_client_id.end()) {
        return nullptr;
    }

    return _slave_by_client_id[client->info.client_id];
}

void Net_master::add_authenticated_slave(Net_client* client, const Net_authenticate_slave& auth) {
    client->info.type = NetClientType::SlaveNode;

    if (_slave_by_slave_id.find(auth.slave_id) != _slave_by_slave_id.end()) {
        printf("[NET-MASTER][NOTICE][Authenticated slave is already registered]\n");
        return;
    }

    // link up the client to a slave_node_info
    auto slave = std::make_shared<Net_slave_info>(client, auth.slave_id);
    _slave_by_slave_id[auth.slave_id] = slave;
    _slave_by_client_id[client->info.client_id] = slave;

    _slaves_health.push_back(slave); // keep it last because we dont know the health yet
    
    printf("[NET-MASTER][New slave node registered successfully]\n");
}

void Net_master::on_inc_tcp_data(Net_client* client, const std::vector<uint8_t>& data, int32_t data_len) {
    
    uint32_t pos = 0;
    int32_t len = data_len;

    while (data_len > 0) {
        MsgType type = (MsgType)((uint8_t)data[pos]);

        if (client->info.type == NetClientType::Unauthenticated) {
            // we only allow authentication packets

            switch (type) {
            case MsgType::NetAuthenticatePlayer:
            {
                Net_authenticate_player auth(data, pos);

                if (auth.client_password == _my_node->client_password) {
                    printf("[NET-MASTER][ON-INC-TCP-DATA][NetAuthenticatePlayer][SUCCESS]\n");
                    client->info.type = NetClientType::Player;

                    pos += sizeof(Net_authenticate_player);
                    len -= sizeof(Net_authenticate_player);
                }
                else {
                    printf("[NET-MASTER][ON-INC-TCP-DATA][NetAuthenticatePlayer][FAIL][Invalid password]\n");
                    _tcp->disconnect(client);
                    return;
                }
                break;
            }
            case MsgType::NetAuthenticateSlave:
            {
                Net_authenticate_slave auth(data, pos);

                if (auth.master_password == _my_node->master_password) {
                    printf("[NET-MASTER][ON-INC-TCP-DATA][NetAuthenticateSlave][SUCCESS]\n");

                    add_authenticated_slave(client, auth);
                }
                else {
                    printf("[NET-MASTER][ON-INC-TCP-DATA][NetAuthenticateSlave][FAIL][Invalid master password]\n");
                    _tcp->disconnect(client);
                    return;
                }

                pos += sizeof(Net_authenticate_slave);
                len -= sizeof(Net_authenticate_slave);
            }
            default:
                // disconnect
                break;
            }

            printf("[NET-MASTER][ON-INC-TCP-DATA][ERROR][Client tried to send message without authenticated state]\n");
            _tcp->disconnect(client);

            return;
        }

        switch (type) {
            case MsgType::NetPlayerRequestSlaveNode: {
                printf("[NET-MASTER][NetPlayerRequestSlaveNode]\n");

                pos += sizeof(Net_player_request_slave_node);
                len -= sizeof(Net_player_request_slave_node);

                // no need to populate packet, its only a uint8
                // just send a slave config
                // But first we need to check that the player has been registered on the master node
                break;
            }
            case MsgType::NetFromSlaveToMasterKeepaliveSession: {
                // Keepalive messages are sent from the slave node to the master to indicate that everything is fine with the session
                printf("[NET-MASTER][NetFromSlaveToMasterKeepaliveSession]\n");

                Net_from_slave_keepalive_sessions keep(data, pos);

                for (auto id : keep.session_ids) {
                    if (_session_id_lookup.find(id) == _session_id_lookup.end()) {
                        printf("[NET-MASTER][ERROR][NetFromSlaveToMasterKeepaliveSession][Session not found]\n");
                        continue;
                    }

                    _session_id_lookup[id]->keepalive();
                }

                pos += sizeof(Net_from_slave_keepalive_sessions);
                len -= sizeof(Net_from_slave_keepalive_sessions);
                break;
            }
            case MsgType::NetSlaveHealthReport:
            {
                printf("[NET-MASTER][NetSlaveHealthReport][Got health report from slave]\n");
                auto slave = get_slave(client);

                if (slave == nullptr) {
                    printf("[NET-MASTER][NetSlaveHealthReport][ERROR][Cant find slave with client_id: %d]\n", client->info.client_id);
                    return;
                }

                Net_slave_health_snapshot health(data, pos);
                printf("Health report from slave_id: %d\n", slave->slave_id);
                slave->set_health_rating(health);
                std::sort(_slaves_health.begin(), _slaves_health.end(), best_health_is_first());

                health.print();

                pos += sizeof(Net_slave_health_snapshot);
                len -= sizeof(Net_slave_health_snapshot);

                break;
            }
            default:
                printf("[NET-MASTER][ON_INC_DATA][ERROR][Unknown message type: %d]\n", type);

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

        if (!_tcp->send_data_to_all(_data_buffer, sizeof(NetMasterToSlaveCommand))) {
            printf("[NET-MASTER][UPDATE][Demand report send failed]\n");
        }
        else {
            printf("[NET-MASTER][UPDATE][Demand report sent]\n");
        }
    }
}

Net_slave_info::Net_slave_info(Net_client* client_, uint32_t slave_id_)
    : client(client_), slave_id(slave_id_), health_rating(-1) {
    
}