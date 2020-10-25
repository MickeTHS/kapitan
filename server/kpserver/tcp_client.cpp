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

bool Tcp_client::send_data(const std::vector<uint8_t>& buffer, int len) {
    size_t bytes_sent;
    
    if ((bytes_sent = send(_socket, (const char*)&buffer[0], len, 0)) != len) {
        printf("[TCP-CLIENT][SEND_DATA][ERROR][Unable to send data]\n");
        print_error();
        return false;
    }

    return true;
}

int Tcp_client::read_data(std::vector<uint8_t>& buffer) {

    if (!_initialized) {
        return 0;
    }

    int valread;

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(_socket, &readfds);

    timeval tt;
    tt.tv_sec = 0;
    tt.tv_usec = 1000;


    int activity = select(_socket, &readfds, NULL, NULL, &tt);

    if (!FD_ISSET(_socket, &readfds)) {
        return 0; // no activity so skip this
    }

    if ((valread = recv(_socket, (char*)&buffer[0], 1024, 0)) <= 0)
    {
        switch (valread) {
            case 0:
                printf("[TCP-CLIENT][READ][DISCONNECT GRACEFULLY]\n");
                closesocket(_socket);
                break;
            case WSAEWOULDBLOCK:
                printf("[TCP-CLIENT][READ][ERROR][WSAEWOULDBLOCK]\n");
                // would block, so no data
                break;
            case WSANOTINITIALISED:
                printf("[TCP-CLIENT][READ][ERROR][WSANOTINITIALISED]\n");
                break;
            case WSAENETDOWN:
                printf("[TCP-CLIENT][READ][ERROR][WSAENETDOWN]\n");
                break;
            case WSAEFAULT:
                printf("[TCP-CLIENT][READ][ERROR][WSAEFAULT]\n");
                break;
            case WSAENOTCONN:
                printf("[TCP-CLIENT][READ][ERROR][WSAENOTCONN]\n");
                break;
            case WSAEINTR:
                printf("[TCP-CLIENT][READ][ERROR][WSAEINTR]\n");
                break;
            case WSAEINPROGRESS:
                printf("[TCP-CLIENT][READ][ERROR][WSAEINPROGRESS]\n");
                break;
            case WSAENETRESET:
                printf("[TCP-CLIENT][READ][ERROR][WSAENETRESET]\n");
                break;
            case WSAENOTSOCK:
                printf("[TCP-CLIENT][READ][ERROR][WSAENOTSOCK]\n");
                break;
            case WSAEOPNOTSUPP:
                printf("[TCP-CLIENT][READ][ERROR][WSAEOPNOTSUPP]\n");
                break;
            case WSAESHUTDOWN:
                printf("[TCP-CLIENT][READ][ERROR][WSAESHUTDOWN]\n");
                break;
            case WSAEMSGSIZE:
                printf("[TCP-CLIENT][READ][ERROR][WSAEMSGSIZE]\n");
                break;
            case WSAEINVAL:
                printf("[TCP-CLIENT][READ][ERROR][WSAEINVAL]\n");
                break;
            case WSAECONNABORTED:
                printf("[TCP-CLIENT][READ][ERROR][WSAECONNABORTED]\n");
                break;
            case WSAETIMEDOUT:
                printf("[TCP-CLIENT][READ][ERROR][WSAETIMEDOUT]\n");
                break;
            case WSAECONNRESET:
                printf("[TCP-CLIENT][READ][ERROR][WSAECONNRESET]\n");
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