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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <stdio.h>
#endif

#include <vector>
#include <stdint.h>
#include <functional>

///
/// Handles the position messages
/// 

struct Pos_socket {
    Pos_socket(int port);
    virtual ~Pos_socket();

    bool init();
    int read();
    int send(const std::vector<uint8_t>& data);
    int get_port() const;

private:
    void print_error();

    int _port;
    int _addrlen;

    struct	sockaddr_in _addr;
    SOCKET _socket;

    std::vector<uint8_t> _recv_buffer;

    std::function<void(const std::vector<uint8_t>&)> _on_data;
};