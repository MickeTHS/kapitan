#include "net_master.h"

#include "tcp_server.h"
#include "net_packet.h"

#include <ctime>

Net_master::Net_master(Tcp_server* tcp, const Ini_file& file) : _ini_file(file) {
    _tcp = tcp;
    _data_buffer.resize(1000);
    _my_node = _ini_file.get_me();
    
    _next_slave_report = std::chrono::high_resolution_clock::now() - std::chrono::minutes(60);
    
    //std::vector<std::shared_ptr<Ini_node>> slaves;
    //file.get_slaves(slaves);

    _tcp->set_on_data_callback([&](std::shared_ptr<Net_client> client, const std::vector<uint8_t>& data) {
        on_inc_data(client, data);
    });
}

Net_master::~Net_master() {

}

void Net_master::on_client_connect(std::shared_ptr<Net_client> client) {
    printf("[NET-MASTER][New slave connected]\n");

    // we must receive a password or we will disconnect the client
}

void Net_master::on_inc_data(std::shared_ptr<Net_client> client, const std::vector<uint8_t>& data) {
    printf("casting %d to %d\n", data[0], (MsgType)((uint8_t)data[0]));
    MsgType type = (MsgType)((uint8_t)data[0]);

    switch (type) {
        case MsgType::NetPlayerRequestSlaveNode: {
            printf("[NET-MASTER][NetPlayerRequestSlaveNode]\n");

            // no need to populate packet, its only a uint8
            // just send a slave config
            // But first we need to check that the player has been registered on the master node
            break;
        }
        case MsgType::NetRegisterPlayer: {
            printf("[NET-MASTER][NetRegisterPlayer]\n");

            Net_register_player reg(data);

            break;
        }
        case MsgType::NetFromSlaveToMasterKeepaliveSession: {
            // Keepalive messages are sent from the slave node to the master to indicate that everything is fine with the session
            printf("[NET-MASTER][NetFromSlaveToMasterKeepaliveSession]\n");

            Net_from_slave_keepalive_sessions keep(data);

            for (auto id : keep.session_ids) {
                if (_session_id_lookup.find(id) == _session_id_lookup.end()) {
                    printf("[NET-MASTER][ERROR][NetFromSlaveToMasterKeepaliveSession][Session not found]\n");
                    continue;
                }

                _session_id_lookup[id]->keepalive();
            }
            break;
        }
        case MsgType::NetSlaveRegisterOnMaster: {
            printf("[NET-MASTER][NetSlaveRegisterOnMaster][Authenticating...]\n");

            Net_slave_register_on_master reg(data);

            if (reg.master_password == _my_node->master_password) {
                printf("[NET-MASTER][NetSlaveRegisterOnMaster][Slave successfully authenticated]\n");
                
                // check if the slave is already registered
                bool found = false;

                for (auto sl : _slaves) {
                    if (sl->slave_id == reg.slave_id) {
                        found = true;
                        break;
                    }
                }

                // link up the client to a slave_node_info
                if (!found) {
                    auto slave = std::make_shared<Net_slave_info>(client, reg.slave_id);
                    _slaves.push_back(slave);

                    printf("[NET-MASTER][New slave node registered successfully]\n");
                }
                else {
                    printf("[NET-MASTER][NOTICE][Authenticated slave is already registered]\n");
                }
            }
            else {
                printf("[NET-MASTER][NetSlaveRegisterOnMaster][FAILED][Mismatched password]\n");
            }
            break;
        }
        default:
            printf("[NET-MASTER][ON_INC_DATA][ERROR][Unknown message type: %d]\n", type);
            break;
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

Net_slave_info::Net_slave_info(std::shared_ptr<Net_client> client_, uint32_t slave_id_) 
    : client(client_), slave_id(slave_id_) {
    
}