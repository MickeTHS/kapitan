#include "net_slave.h"

#include "tcp_server.h"
#include "net_packet.h"
#include "process_stats.h"

Net_slave::Net_slave(Tcp_server* tcp, const Ini_file& file, std::shared_ptr<Process_stats> stats) : _tcp(tcp), _ini_file(file), _stats(stats) {

    _master.node = _ini_file.get_master();
    _my_node = file.get_me();

    _next_slave_report = std::chrono::high_resolution_clock::now() - std::chrono::minutes(60);

    _master_connection = std::make_shared<Tcp_client>();
    _data_buffer.resize(2000);

    _tcp->set_on_data_callback([&](std::shared_ptr<Net_client> client, const std::vector<uint8_t>& data, int32_t len) {
        on_inc_client_tcp_data(client, data, len);
    });

    _tcp->set_on_client_connect_callback([&](std::shared_ptr<Net_client> client) {
        on_client_connect(client);
    });

    _tcp->set_on_client_disconnect_callback([&](std::shared_ptr<Net_client> client) {
        on_client_disconnect(client);
    });
}

Net_slave::~Net_slave() {}

void Net_slave::on_inc_client_tcp_data(std::shared_ptr<Net_client> client, const std::vector<uint8_t>& data, int32_t len) {
    // incoming TCP data from clients/players

    MsgType type = MsgType::None;
    size_t off = 0;
    
    if (client->info.type == NetClientType::Unauthenticated) {
        // we only allow authentication packets
        MsgType type = (MsgType)((uint8_t)data[0]);

        switch (type) {
            case MsgType::NetAuthenticatePlayer:
            {
                Net_authenticate_player auth(data);
                off += sizeof(Net_authenticate_player);

                if (auth.client_password == _my_node->client_password) {
                    printf("[NET-SLAVE][ON-INC-TCP-DATA][NetAuthenticatePlayer][SUCCESS]\n");
                    client->info.type = NetClientType::Player;
                }
                else {
                    printf("[NET-SLAVE][ON-INC-TCP-DATA][NetAuthenticatePlayer][FAIL][Invalid password]\n");
                    _tcp->disconnect(client);
                    return;
                }
                break;
            }
            default:
                // disconnect
                break;
        }

        printf("[NET-SLAVE][ON-INC-TCP-DATA][ERROR][Client tried to send message without authenticated state]\n");
        _tcp->disconnect(client);

        return;
    }

    while (off < len) {
        type = (MsgType)((uint8_t)data[off]);

        switch (type) {
            case MsgType::NetPlayerJoinSession:
            {
                // when a player tries to join a session

                Net_player_join_session sess(data, off);
                off += sizeof(Net_player_join_session);

                mmh::Hash_key key(sess.code);

                if (_session_code_lookup.find(key.hash) != _session_code_lookup.end()) {
                    // we found the session
                }
                else {
                    // session not found
                }
                break;
            }
            case MsgType::NetPlayerSetGameRuleInt:
            {
                // When a gamerule is being set by the owner

                Net_player_set_gamerule_int gamerule(data, off);
                off += sizeof(Net_player_set_gamerule_int);

                break;
            }
        }
    }
}

bool Net_slave::init() {
    // connect to master
    connect_to_master();

    return true;
}

void Net_slave::setup_sessions() {

    for (int i = 0; i < _my_node->max_groups; ++i) {
        std::shared_ptr<Net_session> session = std::make_shared<Net_session>(_my_node->id + (1 + i), _my_node->max_users_per_group, _tcp, _my_node->udp_range_min + i);
        _sessions.push_back(session);
    }
}

void Net_slave::on_client_connect(std::shared_ptr<Net_client> client) {
    // tcp.set_on_new_client_callback([&](std::shared_ptr<Net_client> client) {
    // session->add_client(client);
    // session->send_config();

    // create a new net session player
    auto player = std::make_shared<Net_session_player>();
    player->net_client_id = client->info.client_id;

    _client_id_lookup[client->info.client_id] = player;
}

void Net_slave::on_client_disconnect(std::shared_ptr<Net_client> client) {
    // when a client has disconnected, we need to make sure we delete all references to the client object

    // delete the client
    if (client->info.client_id != 0 && _client_id_lookup.find(client->info.client_id) != _client_id_lookup.end()) {
        _client_id_lookup.erase(client->info.client_id);
    }

    // remove player from the session
    if (client->info.session_id != 0 && _session_id_lookup.find(client->info.session_id) != _session_id_lookup.end()) {
        _session_id_lookup[client->info.session_id]->disconnect(client);
    }
}

void Net_slave::handle_master_command(const Net_master_to_slave_command& command) {
    switch ((NetMasterToSlaveCommand)command.command) {
        case NetMasterToSlaveCommand::ReportHealth:
        {
            printf("[NET-SLAVE][HANDLE-MASTER-COMMAND][Will send health report]\n");

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
                0); // cpu load isnt working

            msg.set_buffer(_data_buffer, 0);
            msg.print();

            if (_master_connection->send_data(_data_buffer, sizeof(Net_slave_health_snapshot))) {
                printf("[NET-SLAVE][HANDLE-MASTER-COMMAND][Health report sent ok]\n");
            }

            break;
        }
        default:
            printf("[NET-SLAVE][HANDLE-MASTER-COMMAND][ERROR][Unknown command: %d]\n", command.command);
            break;
    }
}

void Net_slave::print_stats() {
    Process_stats_snapshot snapshot;

    _stats->gather_stats(snapshot);

    printf(" ---- STATS REPORT -----\n");
    printf("Num good ticks: %lu\nNum lag ticks: %lu\nAverage tick idle time: %ld\nCPU Load process: %f\nCPU Load global: %f\n", 
        snapshot.num_good_ticks, 
        snapshot.num_lag_ticks, 
        snapshot.avg_tick_idle_time, 
        (float)snapshot.cpu_load_process, 
        (float)snapshot.cpu_load_total);
    printf("--- RAM: \n");
    printf("Physical total: %lu KB\nPhysical process used: %lu KB\nPhysical used: %lu KB\nVirtual Process used: %lu KB\nVirtual Total: %lu KB\nVirtual used: %lu KB\n", 
        ((snapshot.ram_phys_total_bytes / 1000)), 
        ((snapshot.ram_phys_process_used_bytes / 1000)),
        ((snapshot.ram_phys_used_bytes / 1000)),
        ((snapshot.ram_virt_process_used_bytes / 1000)),
        ((snapshot.ram_virt_total_bytes / 1000)),
        ((snapshot.ram_virt_used_bytes / 1000)));
}

bool Net_slave::connect_to_master() {
    auto master = _ini_file.get_master();

    if (!_master_connection->init(master->hostname.c_str(), master->port)) {
        printf("[NET-SLAVE][INIT][FATAL ERROR][Unable to init connection to master]\n");
        return false;
    }

    // the first thing we need to send is a authentication packet 
    // that authenticates with the master
    Net_authenticate_slave reg(_my_node->id, _master.node->master_password);
    reg.set_buffer(_data_buffer, 0);

    for (int i = 0; i < 100; ++i) {
        if (!_master_connection->send_data(_data_buffer, sizeof(Net_authenticate_slave))) {
            //printf("[NET-SLAVE][INIT][ERROR][Unable to authenticate with the master]\n");
            //return false;
        }
    }

    printf("[NET-SLAVE][INIT][Authentication packet sent]\n");

    return true;
}

void Net_slave::update() {
    // make sure we have a connection active to the master server
    if (!_master_connection->is_initialized()) {
        if (!connect_to_master()) {
            return;
        }
    }

    // get data from the master socket

    uint32_t bytesread = _master_connection->read_data(_data_buffer);

    if (bytesread > 0) {
        MsgType type = (MsgType)_data_buffer[0];

        switch (type) {
            case MsgType::NetFromMasterToSlaveCommand:
            {
                Net_master_to_slave_command cmd(_data_buffer, 0);
                handle_master_command(cmd);
                break;
            }
            default: 
                printf("[NET-SLAVE][UPDATE][ERROR][Unknown message type: %d]\n", type);
                break;
        }
    }

    if (_next_slave_report < std::chrono::high_resolution_clock::now()) {
        //print_stats();
        _next_slave_report = std::chrono::high_resolution_clock::now() + std::chrono::seconds(5);
    }
}