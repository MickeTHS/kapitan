#include "tcp_server.h"

#include <iostream>
#include <thread>
#include <errno.h>


#define FD_INVALID -1

void connection_info(struct sockaddr_in& client, Net_client_info& info)
{
    char* connected_ip = inet_ntoa(client.sin_addr);
    int port = ntohs(client.sin_port);

    std::cout << "-[IP:" << connected_ip << ", Connected on PORT:" << port << "]" << std::endl;

    info.int_ip = ntohl(client.sin_addr.s_addr);
    info.ip = std::string(connected_ip);
    info.port = port;
}


Tcp_server::Tcp_server(int con_port) 
    :   _max_clients(10000),
        _con_port(con_port),
        _clear_tick_counter(0),
        _master_socket(0),
        _addrlen(0),
        _on_connect(nullptr),
        _on_disconnect(nullptr),
        _address({ 0 })
{
    _address.sin_family = AF_INET;
    _address.sin_addr.s_addr = INADDR_ANY;
    _address.sin_port = htons(_con_port);

    _data_buffer.resize(2000);
}

Tcp_server::~Tcp_server() {

}

void Tcp_server::set_on_client_connect_callback(std::function<void(std::shared_ptr<Net_client>)> func) {
    _on_connect = func;
}

void Tcp_server::set_on_client_disconnect_callback(std::function<void(std::shared_ptr<Net_client>)> func) {
    _on_disconnect = func;
}

void Tcp_server::set_on_data_callback(std::function<void(std::shared_ptr<Net_client>, const std::vector<uint8_t>& data, int32_t len)> func) {
    _on_data = func;
}

void Tcp_server::print_error() {
#ifdef WIN32
    wchar_t* s = NULL;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR)&s, 0, NULL);
    fprintf(stderr, "%S\n", s);
    LocalFree(s);
#else
    printf("Error: %s\n", strerror(errno));
#endif
}

bool Tcp_server::init() {
    printf("[CON-TCP][INIT]\n");

    int new_socket, i;

#ifdef WIN32
    int opt = TRUE;
    
    WSADATA wsaData;

    int wsa_result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsa_result != 0) {
        
        printf("[CON-TCP][INIT][FAIL][WSASTARTUP]:\n");
        print_error();

        return false;
    }
#else 
    int opt = 1;
#endif

    //create a master socket  
    if ((_master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        printf("[CON-TCP][INIT][FAIL][SOCKET]:\n");
        print_error();
        exit(EXIT_FAILURE);
    }

    //set master socket to allow multiple connections ,  
    //this is just a good habit, it will work without this  
    if (setsockopt(_master_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt,
        sizeof(opt)) < 0)
    {
        printf("[CON-TCP][INIT][FAIL][REUSEADDR]:\n");
        print_error();

        exit(EXIT_FAILURE);
    }

    //bind the socket to localhost port con_port  
    if (bind(_master_socket, (struct sockaddr*)&_address, sizeof(_address)) < 0)
    {
        printf("[CON-TCP][INIT][FAIL][BIND]:\n");
        print_error();

        exit(EXIT_FAILURE);
    }
    
    //try to specify maximum of 100 pending connections for the master socket  
    if (listen(_master_socket, 100) < 0)
    {
        printf("[CON-TCP][INIT][FAIL][LISTEN]:\n");
        print_error();

        exit(EXIT_FAILURE);
    }

    int option = 1;
#ifdef WIN32
    if (setsockopt(_master_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&option, sizeof(option)) < 0) {
        //WSACleanup();
        printf("[CON-TCP][INIT][FAIL][REUSEADDR]:\n");
        print_error();
        return false;
    }
#else 
    setsockopt(_master_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
#endif 

    _addrlen = sizeof(_address);
    printf("[CON-TCP][INIT][OK][p: %d]\n", _con_port);

    return true;
}


int Tcp_server::read() {
    // clear our block list every once in a while
    if (_clear_tick_counter++ > 100) {
        _blocked_clients.clear();
        _clear_tick_counter = 0;
    }

    //set of socket descriptors
    fd_set readfds;
    int max_sd;
    int i;
    int activity;
    int valread;

    SOCKET sd;
    SOCKET new_socket;

    char buffer[1025];  //data buffer of 1K  
    
    FD_ZERO(&readfds);

    //add master socket to set  
    FD_SET(_master_socket, &readfds);
    max_sd = _master_socket;

    for (auto client : _clients) {
        sd = client->get_tcp_socket();

        if (sd > 0) {
            FD_SET(sd, &readfds);
        }
            
        //highest file descriptor number, need it for the select function  
        if (sd > max_sd) {
            max_sd = sd;
        }
    }

    timeval tt;
    tt.tv_sec = 0;
    tt.tv_usec = 1000;

    //wait for an activity on one of the sockets , timeout is NULL ,  
    //so wait indefinitely  
    activity = select(max_sd + 1, &readfds, NULL, NULL, &tt);


    if ((activity < 0) && (errno != EINTR))
    {
        printf("[CON-TCP][READ][SELECT][FAIL][h:%s][p:%d][l:%d]\n");
        print_error();
    }

    //If something happened on the master socket ,  
    //then its an incoming connection  
    if (FD_ISSET(_master_socket, &readfds))
    {
        if ((new_socket = accept(_master_socket,
            (struct sockaddr*)&_address, (socklen_t*)&_addrlen)) < 0)
        {
            printf("[CON-TCP][READ][ACCEPT][FAIL][h:%s][p:%d][l:%d]\n");
            exit(EXIT_FAILURE);
        }

        //inform user of socket number - used in send and receive commands  
        //printf("New connection, socket fd is %d , ip is : %s , port : %d \n" , new_socket, inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

        Net_client_info info;

        connection_info(_address, info);
        info.tcp_socket = new_socket;

        if (_blocked_clients.find(info.int_ip) != _blocked_clients.end()) {
            printf("Client is blocked\n");
#ifdef WIN32
            shutdown(new_socket, SD_BOTH);
            closesocket(new_socket);
#else
            close(new_socket);
#endif
        }
        else {
            printf("[CON-TCP][READ][NEW-CONNECTION][OK]");
            info.print();

            std::shared_ptr<Net_client> client = std::make_shared<Net_client>(info, Net_client::__ID_COUNTER++);
            _clients.push_back(client);

            if (_on_connect != nullptr) {
                _on_connect(client);
            }
        }
    }

    //for (auto client : _clients) {
    for (int i = 0; i < _clients.size(); ++i) {
        auto client = _clients[i];

        sd = client->get_tcp_socket();

        if (FD_ISSET(sd, &readfds)) {
            valread = recv(sd, (char*)&_data_buffer[0], 1024, 0);

            switch (valread) {
                case -1:
                case 0:
#ifdef WIN32
                case WSAENOTCONN:
                case WSAENOTSOCK:
                case WSAESHUTDOWN:
                case WSAECONNABORTED:
                case WSAECONNRESET:
#endif
                    printf("disconnected\n");
                    disconnect(client);
                    --i;
                    continue;
            }

            if (valread == 1024) { // force shutdown for buffer overflow
                disconnect(client);
                _blocked_clients[client->info.int_ip] = true;
                --i;
                continue;
            }

            if (valread > 0) {
                printf("read: %d\n", valread);
                if (!client->log_activity()) { // force shutdown for package flooding
                    printf("Disconnect due to packet flooding\n");
                    disconnect(client);
                    _blocked_clients[client->info.int_ip] = true;
                    --i;
                    continue;
                }

                // parse the message
                if (_on_data != nullptr) {
                    _on_data(client, _data_buffer, valread);
                }
            }
        }
    }

    return 0;
}

void Tcp_server::disconnect(std::shared_ptr<Net_client> client) {
    // destructor of Net_client deals with disconnects
    // closesocket(client->get_socket());

    for (int i = 0; i < _clients.size(); ++i) {
        if (_clients[i] == client) {
            _clients.erase(_clients.begin() + i);
            return;
        }
    }

    if (_on_disconnect != nullptr) {
        _on_disconnect(client);
    }
}

void Tcp_server::send_client_data() {
    for (auto client : _clients) {
        client->send_tcp_data();
    }
}

bool Tcp_server::send_data_to_all(const std::vector<uint8_t>& data, size_t len) {
    SOCKET sd;
    fd_set readfds;
    size_t bytes_sent;
    bool success = true;

    for (auto client : _clients) {
        sd = client->get_tcp_socket();
        if ((bytes_sent = send(sd, (const char*)&data[0], len, 0)) != len) {
            printf("[CON-TCP][SEND_DATA_TO_ALL][ERROR][Unable to send to client socket]\n");
            success = false;
        }
    }

    return success;
}