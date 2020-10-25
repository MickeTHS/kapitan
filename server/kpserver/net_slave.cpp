#include "net_slave.h"

#include "tcp_server.h"
#include "net_packet.h"
#include "process_stats.h"

Net_slave::Net_slave(Tcp_server* tcp, const Ini_file& file, std::shared_ptr<Process_stats> stats) : _tcp(tcp), _ini_file(file), _stats(stats) {

    _master.node = _ini_file.get_master();

    _next_slave_report = std::chrono::high_resolution_clock::now() - std::chrono::minutes(60);

    _master_connection = std::make_shared<Tcp_client>();
    _data_buffer.resize(2000);

    _tcp->set_on_data_callback([&](std::shared_ptr<Net_client> client, const std::vector<uint8_t>& data) {
        on_inc_data(client, data);
    });
}

Net_slave::~Net_slave() {}

void Net_slave::on_inc_data(std::shared_ptr<Net_client> client, const std::vector<uint8_t>& data) {

}

bool Net_slave::init() {
    // connect to master
    auto master = _ini_file.get_master();

    if (!_master_connection->init(master->hostname.c_str(), master->port)) {
        printf("[NET-SLAVE][INIT][FATAL ERROR][Unable to init connection to master]\n");
        return false;
    }

    // the first thing we need to send is a registration packet 
    // that authenticates with the master
    Net_slave_register_on_master reg(_master.node->id, _master.node->master_password);
    reg.set_buffer(_data_buffer, 0);
    printf("msgtype:%d\n",_data_buffer[0]);

    if (!_master_connection->send_data(_data_buffer, sizeof(Net_slave_register_on_master))) {
        printf("[NET-SLAVE][INIT][ERROR][Unable to authenticate with the master]\n");
        return false;
    }

    printf("[NET-SLAVE][INIT][Authentication packet sent]\n");

    return true;
}

void Net_slave::handle_master_command(const Net_master_to_slave_command& command) {
    switch ((NetMasterToSlaveCommand)command.command) {
        case NetMasterToSlaveCommand::ReportHealth:
            printf("[NET-SLAVE][HANDLE-MASTER-COMMAND][Will send health report]\n");
            break;
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

void Net_slave::update() {
    // read from the master socket
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