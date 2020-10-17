#include "con_socket.h"

#include <iostream>
#include <thread>

#define FD_INVALID -1

void connection_info(struct sockaddr_in& client, Net_client_info& info)
{
    char* connected_ip = inet_ntoa(client.sin_addr);
    int port = ntohs(client.sin_port);

    std::cout << "-[IP:" << connected_ip << ", Connected on PORT:" << port << "]" << std::endl;

    info.ip = std::string(connected_ip);
    info.port = port;
}


Con_socket::Con_socket(int con_port) {
    _con_port = con_port;
    _on_new_client = nullptr;
}

Con_socket::~Con_socket() {

}

void Con_socket::set_on_new_client_callback(std::function<void(std::shared_ptr<Net_client>)> func) {
    _on_new_client = func;
}


void Con_socket::print_error() {
    wchar_t* s = NULL;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR)&s, 0, NULL);
    fprintf(stderr, "%S\n", s);
    LocalFree(s);
}

bool Con_socket::init() {
    int opt = TRUE;
    int new_socket, i;
    _max_clients = 30;

    //initialise all client_socket[] to 0 so not checked  
    for (i = 0; i < _max_clients; i++)
    {
        _client_socket[i] = 0;
    }

#ifdef WIN32
    WSADATA wsaData;

    int wsa_result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsa_result != 0) {
        printf("WSAStartup failed with error: %d\n", wsa_result);
        return false;
    }
#endif

    //create a master socket  
    if ((_master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        print_error();
        exit(EXIT_FAILURE);
    }

    //set master socket to allow multiple connections ,  
    //this is just a good habit, it will work without this  
    if (setsockopt(_master_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt,
        sizeof(opt)) < 0)
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    //type of socket created  
    _address.sin_family = AF_INET;
    _address.sin_addr.s_addr = INADDR_ANY;
    _address.sin_port = htons(_con_port);

    //bind the socket to localhost port 8888  
    if (bind(_master_socket, (struct sockaddr*)&_address, sizeof(_address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Listener on port %d \n", _con_port);

    //try to specify maximum of 3 pending connections for the master socket  
    if (listen(_master_socket, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    int option = 1;
#ifdef WIN32
    if (setsockopt(_master_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&option, sizeof(option)) < 0) {
        WSACleanup();

        return false;
    }
#else 
    setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

#endif 

    //accept the incoming connection  
    _addrlen = sizeof(_address);
    puts("Waiting for connections ...");

    return true;
}

int Con_socket::sendto_client(std::shared_ptr<Net_client> client, const std::vector<uint8_t>& data) {

    if (send(client->get_socket(), (const char*)&data[0], data.size(), 0) != data.size())
    {
        perror("send");
    }

    return 0;
}

int Con_socket::read(int con_port) {
    //set of socket descriptors  
    fd_set readfds;
    int max_sd;
    int i;
    int activity;
    int valread;

    SOCKET sd;
    SOCKET new_socket;

    char buffer[1025];  //data buffer of 1K  
    //a message  
    char* message = "ECHO Daemon v1.0 \r\n";


    //while (TRUE)
    //{
        //clear the socket set  
        FD_ZERO(&readfds);

        //add master socket to set  
        FD_SET(_master_socket, &readfds);
        max_sd = _master_socket;

        //add child sockets to set  
        for (i = 0; i < _max_clients; i++)
        {
            //socket descriptor  
            sd = _client_socket[i];

            //if valid socket descriptor then add to read list  
            if (sd > 0)
                FD_SET(sd, &readfds);

            //highest file descriptor number, need it for the select function  
            if (sd > max_sd)
                max_sd = sd;
        }

        timeval tt;
        tt.tv_sec = 0;
        tt.tv_usec = 1000;

        //wait for an activity on one of the sockets , timeout is NULL ,  
        //so wait indefinitely  
        activity = select(max_sd + 1, &readfds, NULL, NULL, &tt);


        if ((activity < 0) && (errno != EINTR))
        {
            printf("select error");
        }

        //If something happened on the master socket ,  
        //then its an incoming connection  
        if (FD_ISSET(_master_socket, &readfds))
        {
            if ((new_socket = accept(_master_socket,
                (struct sockaddr*)&_address, (socklen_t*)&_addrlen)) < 0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            //inform user of socket number - used in send and receive commands  
            //printf("New connection, socket fd is %d , ip is : %s , port : %d \n" , new_socket, inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

            Net_client_info info;

            connection_info(_address, info);
            info.socket = new_socket;

            printf("New connection: ");
            info.print();

            std::shared_ptr<Net_client> client = std::make_shared<Net_client>(info);
            _clients.push_back(client);

            if (_on_new_client != nullptr) {
                _on_new_client(client);
            }

            //send new connection greeting message  
            if (send(new_socket, message, strlen(message), 0) != strlen(message))
            {
                perror("send");
            }

            puts("Welcome message sent successfully");

            //add new socket to array of sockets  
            for (i = 0; i < _max_clients; i++)
            {
                //if position is empty  
                if (_client_socket[i] == 0)
                {
                    _client_socket[i] = new_socket;
                    printf("Adding to list of sockets as %d\n", i);

                    break;
                }
            }
        }

        //else its some IO operation on some other socket 
        for (i = 0; i < _max_clients; i++)
        {
            sd = _client_socket[i];

            if (FD_ISSET(sd, &readfds))
            {
                //Check if it was for closing , and also read the  
                //incoming message  
                if ((valread = recv(sd, buffer, 1024, 0)) == 0)
                {
                    //Somebody disconnected , get his details and print  
                    getpeername(sd, (struct sockaddr*)&_address, (socklen_t*)&_addrlen);
                    printf("Host disconnected , ip %s , port %d \n", inet_ntoa(_address.sin_addr), ntohs(_address.sin_port));

                    //Close the socket and mark as 0 in list for reuse  
                    closesocket(sd);
                    _client_socket[i] = 0;
                }

                //Echo back the message that came in  
                else
                {
                    //set the string terminating NULL byte on the end  
                    //of the data read  
                    buffer[valread] = '\0';
                    send(sd, buffer, strlen(buffer), 0);
                }
            }
        }
    //}

    return 0;
}
