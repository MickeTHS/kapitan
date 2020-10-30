#include "tcp_client.h"
#include "trace.h"

Tcp_client::Tcp_client() 
    :   _socket(0), 
        _initialized(false),
        _on_data(nullptr) {

    _data_buffer_pos = 0;
    _data_buffer.resize(10000);
}

Tcp_client::~Tcp_client() {
#ifdef WIN32
    WSACleanup();
#endif
}

bool Tcp_client::init(const char* ip, const char* hostname, bool is_ip_set, int port) {
    TRACE("[TCP-CLIENT][INIT][%s][%d]\n", (is_ip_set ? ip : hostname), port);
    _socket = 0;

    struct sockaddr_in serv_addr; 
    
#ifdef WIN32
    WSADATA wsaData;

    int wsa_result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsa_result != 0) {

        TRACE("[TCP-CLIENT][INIT][FAIL][WSASTARTUP]:\n");
        
        return false;
    }
#endif

    struct hostent* he;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if ((_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        TRACE("[TCP-CLIENT][INIT][ERROR][Unable to create socket]\n");
        return false;
    }

    if (is_ip_set) {
        if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0)
        {
            TRACE("[TCP-CLIENT][INIT][ERROR][Unable to format IP address]\n");
            return false;
        }
    }
    else {
        // resolve hostname
        if ((he = gethostbyname(hostname)) == NULL) {
            TRACE("[TCP-CLIENT][INIT][ERROR][Hostname lookup failed]\n");
            return false;
        }

        memcpy(&serv_addr.sin_addr, he->h_addr_list[0], he->h_length);

#ifdef WIN32
        inet_ntop(AF_INET, &serv_addr.sin_addr, (PSTR)ip, strlen(ip));
#else
        inet_ntop(AF_INET, &serv_addr.sin_addr, (char*)ip, strlen(ip));
#endif
    }

    TRACE("[TCP-CLIENT][INIT][Ip of remote server: %s]\n", ip);
   
    if (connect(_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    { 
        TRACE("[TCP-CLIENT][INIT][ERROR][Connection error]\n");
        print_error();

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
    TRACE("%S\n", s);
    LocalFree(s);
#endif
}

void Tcp_client::add_data(void* data, int32_t len) {
    memcpy(&_data_buffer[_data_buffer_pos], data, len);
    _data_buffer_pos += len;
}

void Tcp_client::send_buffer() {
    size_t bytes_sent;

    uint32_t pos = 0;
    int32_t len = (int32_t)_data_buffer_pos;

    while (len > 0) {
        bytes_sent = send(_socket, (const char*)&_data_buffer[pos], len, 0);

        if (bytes_sent == SOCKET_ERROR) {
            /*result = WSAGetLastError();

            if (result != WSAEWOULDBLOCK) {
                // ignore
            }
            */
            _data_buffer_pos = 0;
            return;
        }
        else {
            len -= bytes_sent;
            pos += bytes_sent;
        }
    }

    _data_buffer_pos = 0;
}

void Tcp_client::set_on_data_callback(std::function<void(const std::vector<uint8_t>& data, int32_t len)> func) {
    _on_data = func;
}

void Tcp_client::update() {
    read_data(_data_buffer);
    send_buffer();
}

int32_t Tcp_client::read_data(std::vector<uint8_t>& buffer) {

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
#ifdef WIN32
        switch (valread) {
            case 0:
                TRACE("[TCP-CLIENT][READ][DISCONNECT GRACEFULLY]\n");
                closesocket(_socket);
                break;
            case WSAEWOULDBLOCK:
                TRACE("[TCP-CLIENT][READ][ERROR][WSAEWOULDBLOCK]\n");
                // would block, so no data
                break;
            case WSANOTINITIALISED:
                TRACE("[TCP-CLIENT][READ][ERROR][WSANOTINITIALISED]\n");
                break;
            case WSAENETDOWN:
                TRACE("[TCP-CLIENT][READ][ERROR][WSAENETDOWN]\n");
                break;
            case WSAEFAULT:
                TRACE("[TCP-CLIENT][READ][ERROR][WSAEFAULT]\n");
                break;
            case WSAENOTCONN:
                TRACE("[TCP-CLIENT][READ][ERROR][WSAENOTCONN]\n");
                break;
            case WSAEINTR:
                TRACE("[TCP-CLIENT][READ][ERROR][WSAEINTR]\n");
                break;
            case WSAEINPROGRESS:
                TRACE("[TCP-CLIENT][READ][ERROR][WSAEINPROGRESS]\n");
                break;
            case WSAENETRESET:
                TRACE("[TCP-CLIENT][READ][ERROR][WSAENETRESET]\n");
                break;
            case WSAENOTSOCK:
                TRACE("[TCP-CLIENT][READ][ERROR][WSAENOTSOCK]\n");
                break;
            case WSAEOPNOTSUPP:
                TRACE("[TCP-CLIENT][READ][ERROR][WSAEOPNOTSUPP]\n");
                break;
            case WSAESHUTDOWN:
                TRACE("[TCP-CLIENT][READ][ERROR][WSAESHUTDOWN]\n");
                break;
            case WSAEMSGSIZE:
                TRACE("[TCP-CLIENT][READ][ERROR][WSAEMSGSIZE]\n");
                break;
            case WSAEINVAL:
                TRACE("[TCP-CLIENT][READ][ERROR][WSAEINVAL]\n");
                break;
            case WSAECONNABORTED:
                TRACE("[TCP-CLIENT][READ][ERROR][WSAECONNABORTED]\n");
                break;
            case WSAETIMEDOUT:
                TRACE("[TCP-CLIENT][READ][ERROR][WSAETIMEDOUT]\n");
                break;
            case WSAECONNRESET:
                TRACE("[TCP-CLIENT][READ][ERROR][WSAECONNRESET]\n");
                break;
            default:
                // fault on the master, try to reconnect
                disconnect();
                TRACE("[TCP-CLIENT][READ][ERROR][%d]\n", valread);
                print_error();
                break;
        }
#else
        switch (valread) {
            case -1:
            case 0:
                disconnect();
        }
#endif
    }
    else 
    {
        // got data
        if (_on_data != nullptr) {
            _on_data(buffer, valread);
        }

        return valread;
    }

    return 0;
}

void Tcp_client::disconnect() {
#ifdef WIN32
    //WSACleanup();
    shutdown(_socket, SD_SEND);
    closesocket(_socket);
#else
    close(_socket);
#endif

    _initialized = false;
}

bool Tcp_client::is_initialized() const {
    return _initialized;
}