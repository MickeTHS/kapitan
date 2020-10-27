#pragma once

#ifdef WIN32

#define NOMINMAX

#pragma comment(lib, "Ws2_32.lib")

#define WIN32_LEAN_AND_MEAN 1
#include <winsock2.h>
#include <winsock.h>
#include <windows.h>
#include <Ws2tcpip.h>
#else


#ifndef SOCKET
#define SOCKET int32_t
#endif


#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>


#endif

#include <vector>
#include <stdint.h>
#include <functional>
#include <memory>
#include <unordered_map>
#include "net_client.h"
#include "net_packet.h"

///
/// Handles the position messages
/// 

struct Net_session_player;

struct Udp_server {
    Udp_server();
    virtual ~Udp_server();

    bool init(int port);
    int read();
    int send(const std::vector<Net_client*>& clients, const std::vector<uint8_t>& data, uint32_t len);
    int session_send(const std::vector<Net_session_player*>& players, int num_players, const std::vector<uint8_t>& data, uint32_t data_len);
    int get_port() const;
    void set_on_client_data_callback(std::function<void(Net_client*, const std::vector<uint8_t>&, int32_t)> func);
    void set_on_client_connect_callback(std::function<void(Net_client*, const Net_Udp_establish&)> func);
    void establish_client_connection(Net_client* client);
    void remove_client(Net_client* client);

private:
    void print_error();
    
    int _port;
    int _addrlen;

    struct	sockaddr_in _addr;
    SOCKET _socket;

    std::vector<uint8_t> _recv_buffer;

    std::function<void(Net_client*, const Net_Udp_establish&)> _on_client_connect;
    std::function<void(Net_client*, const std::vector<uint8_t>&, int32_t)> _on_client_data;

    std::unordered_map<uint64_t, Net_client*> _client_hash_map;
    std::unordered_map<uint32_t, Net_client*> _client_id_map;
};