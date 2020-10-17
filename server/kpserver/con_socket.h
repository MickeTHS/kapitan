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

#include <stdio.h>
#include <string.h>    //strlen
#include <stdlib.h>
#include <fcntl.h>
#include <vector>
#include <memory>
#include <functional>

#include "net_client.h"

#define CON_SOCKET_DESCR 0
#define CHAT_SOCKET_DESCR 1

struct Con_socket {
    Con_socket(int con_port);
    virtual ~Con_socket();

    bool init();
    int read(int con_port);
    int sendto_client(std::shared_ptr<Net_client> client, const std::vector<uint8_t>& data);

    void print_error();

    void set_on_new_client_callback(std::function<void(std::shared_ptr<Net_client>)> func);

    int _con_port;
    
    SOCKET _master_socket;
    int _max_clients;
    SOCKET _client_socket[30];
    struct sockaddr_in _address;
    int _addrlen;

    std::vector<std::shared_ptr<Net_client>> _clients;

    std::function<void(std::shared_ptr<Net_client>)> _on_new_client;
};