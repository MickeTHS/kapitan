#include "tcp_client.h"

Tcp_client::Tcp_client() : _socket(0), _initialized(false) {}

bool Tcp_client::init(const char* hostname, int port) {
    _socket = 0;

    struct sockaddr_in serv_addr; 
    
#ifdef WIN32
    WSADATA wsaData;

    int wsa_result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsa_result != 0) {

        printf("[TCP-CLIENT][INIT][FAIL][WSASTARTUP]:\n");
        
        return false;
    }
#endif

    struct hostent* he;

    // resolve hostname
    if ((he = gethostbyname(hostname)) == NULL) {
        printf("[TCP-CLIENT][INIT][ERROR][Hostname lookup failed]\n");
        return false;
    }

    if ((_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    { 
        printf("[TCP-CLIENT][INIT][ERROR][Unable to create socket]\n"); 
        return false;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    memcpy(&serv_addr.sin_addr, he->h_addr_list[0], he->h_length);
    
    char ip[64];

    inet_ntop(AF_INET, &serv_addr.sin_addr, ip, sizeof(ip));

    printf("[TCP-CLIENT][INIT][Ip of remote server: %s]\n", ip);

    // Convert IPv4 and IPv6 addresses from text to binary form 
/*  if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0)  
    { 
        printf("[TCP-CLIENT][INIT][ERROR][Unable to format IP address]\n"); 
        return false; 
    } */
   
    if (connect(_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    { 
        printf("[TCP-CLIENT][INIT][ERROR][Connection error]\n");
        print_error();
        
        WSACleanup();

        return false; 
    } 

#ifdef WIN32
    unsigned long mode = 1;

    ioctlsocket(_socket, FIONBIO, &mode);
#else
    int status = fcntl(_socket, F_SETFL, fcntl(_socket, F_GETFL, 0) | O_NONBLOCK);

    if (status == -1) {
        perror("calling fcntl");
        // handle the error.  By the way, I've never seen fcntl fail in this way
    }

#endif
    
    _initialized = true;

    return true;
}


void Tcp_client::print_error() {

#ifdef WIN32
    wchar_t* s = NULL;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR)&s, 0, NULL);
    fprintf(stderr, "%S\n", s);
    LocalFree(s);
#endif
}

int Tcp_client::read_data(std::vector<uint8_t>& buffer) {

    if (!_initialized) {
        return 0;
    }

    int valread;

    if ((valread = recv(_socket, (char*)&buffer[0], 1024, 0)) <= 0)
    {
        switch (valread) {
            case 0:
                printf("[TCP-CLIENT][READ][DISCONNECT GRACEFULLY]\n");
                closesocket(_socket);
                break;
            case WSAEWOULDBLOCK:
                //printf("[TCP-CLIENT][READ][ERROR][WSAEWOULDBLOCK]\n");
                // would block, so no data
                break;
            default:
                printf("[TCP-CLIENT][READ][ERROR][%d]\n", valread);
                print_error();
                break;
        }
    }
    else 
    {
        // got data
        return valread;
    }

    return 0;
}