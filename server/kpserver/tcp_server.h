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

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>

#endif


#include <stdint.h>
#include <stdio.h>
#include <string.h>    //strlen
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

struct Tcp_server {
    Tcp_server(int con_port);
    virtual ~Tcp_server();

    bool init();
    int read();
    void send_client_data();

    void set_on_client_connect_callback(std::function<void(std::shared_ptr<Net_client>)> func);
    void set_on_client_disconnect_callback(std::function<void(std::shared_ptr<Net_client>)> func);
    void set_on_data_callback(std::function<void(std::shared_ptr<Net_client>, const std::vector<uint8_t>& data, int32_t len)> func);
    bool send_data_to_all(const std::vector<uint8_t>& data, size_t len);
    void disconnect(std::shared_ptr<Net_client> client);
private:
    void print_error();

    int _con_port;
    
    SOCKET _master_socket;
    
    struct sockaddr_in _address;
    int _addrlen;
    
    int _max_clients;
    int _clear_tick_counter;

    std::vector<std::shared_ptr<Net_client>> _clients;

    std::function<void(std::shared_ptr<Net_client>)> _on_connect;
    std::function<void(std::shared_ptr<Net_client>)> _on_disconnect;
    std::function<void(std::shared_ptr<Net_client>, const std::vector<uint8_t>& data, int32_t len)> _on_data;

    std::unordered_map<uint64_t, bool> _blocked_clients;

    std::vector<uint8_t> _data_buffer;
};