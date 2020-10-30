#pragma once

#include <vector>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <functional>

#ifdef WIN32

#define NOMINMAX

#pragma comment(lib, "Ws2_32.lib")

#define WIN32_LEAN_AND_MEAN 1
#include <winsock2.h>
#include <winsock.h>
#include <windows.h>
#include <Ws2tcpip.h>
#include <stdio.h>
#else
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

#ifndef SOCKET
#define SOCKET int32_t
#endif

#endif

/// <summary>
/// A simple non blocking TCP client, currently only used by
/// node slaves to connect to the master node
/// </summary>
struct Tcp_client {
    Tcp_client();
    virtual ~Tcp_client();

    bool init(const char* ip, const char* hostname, bool is_ip_set, int port);

    void add_data(void* data, int32_t len);

    void send_buffer();

    void disconnect();

    bool is_initialized() const;

    void set_on_data_callback(std::function<void(const std::vector<uint8_t>& data, int32_t len)> func);

    void update();

private:
    void print_error();

    int32_t read_data(std::vector<uint8_t>& buffer);

    SOCKET _socket;

    bool _initialized;
   
    std::vector<uint8_t> _data_buffer;

    uint32_t _data_buffer_pos;

    std::function<void(const std::vector<uint8_t>& data, int32_t len)> _on_data;
};