#include "net_master.h"

#include "con_socket.h"
#include "net_packet.h"

Net_master::Net_master(Con_socket* tcp) {
    _tcp = tcp;

    _tcp->set_on_data_callback([&](std::shared_ptr<Net_client> client, const std::vector<uint8_t>& data) {
        on_inc_data(client, data);
    });
}

Net_master::~Net_master() {

}

void Net_master::on_inc_data(std::shared_ptr<Net_client> client, const std::vector<uint8_t>& data) {
    MsgType type = (MsgType)((uint8_t)&data[0]);

    switch (type) {
        case MsgType::NetPlayerRequestSlaveNode:
            
            printf("[NET-MASTER][NetPlayerRequestSlaveNode]\n");
            break;
        default:
            break;
    }
}