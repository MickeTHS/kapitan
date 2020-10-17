#include "net_packet.h"

Net_packet::Net_packet(uint32_t size) {
    data.resize(size);
}

Net_packet::~Net_packet() {}

