#pragma once

#include <vector>
#include <stdint.h>

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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <stdio.h>

#endif


struct Tcp_client {
    Tcp_client();

    bool init(const char* hostname, int port);
    int read_data(std::vector<uint8_t>& buffer);
    bool send_data(const std::vector<uint8_t>& buffer, int len);

private:
    void print_error();

    SOCKET _socket;
    bool _initialized;
};