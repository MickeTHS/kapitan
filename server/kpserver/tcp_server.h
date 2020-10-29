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


#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <unordered_map>

#include "net_client.h"

/// http://beej.us/guide/bgnet/html/

#define CON_SOCKET_DESCR 0
#define CHAT_SOCKET_DESCR 1

/// <summary>
/// A Tcp server used by both the slaves and the master
/// </summary>
struct Tcp_server {
    Tcp_server();
    virtual ~Tcp_server();

    bool init(uint16_t port);
    int read();
    void send_client_data();

    void set_on_client_connect_callback(std::function<void(Net_client*)> func);

    void set_on_client_disconnect_callback(std::function<void(Net_client*)> func);

    void set_on_client_data_callback(std::function<void(Net_client*, const std::vector<uint8_t>& data, int32_t len)> func);

    bool send_data_to_all(const std::vector<uint8_t>& data, size_t len);

    void disconnect(Net_client* client);
private:
    void print_error();

    uint16_t _port;
    
    SOCKET _master_socket;
    
    struct sockaddr_in _address;
    int _addrlen;
    
    int _max_clients;
    int _clear_tick_counter;

    std::vector<std::unique_ptr<Net_client>> _clients;

    std::function<void(Net_client*)> _on_connect;
    std::function<void(Net_client*)> _on_disconnect;
    std::function<void(Net_client*, const std::vector<uint8_t>& data, int32_t len)> _on_data;

    std::unordered_map<uint64_t, bool> _blocked_clients;

    std::vector<uint8_t> _data_buffer;
};