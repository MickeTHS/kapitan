#pragma once

#include <stdint.h>
#include <string>
#include <vector>

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


#define MSG_BUF_SIZE 20000

struct Net_client_info {
    std::string ip;
    int port;
	SOCKET socket;

	void print() const {
		printf("[NET-CLIENT][fd: %d][p: %d][ip: %s]\n", socket, port, ip.c_str());
	}
};

struct Net_client {
    Net_client(Net_client_info info);
	virtual ~Net_client();

	
	void handle_rec_packet(uint8_t* data, int msglen);
	bool init();
	void read();
	std::string get_ip() const;

    void print_byte_buffer(uint8_t *buffer, int length);

    int mseconds_since_activity() const;
    SOCKET get_socket() const;

	void print() const;
	//void close_socket();

    std::vector<int>    queue;

private:
	
    Net_client_info     _info;
	
	struct	sockaddr_in _addr;
    struct  ip_mreq     _mreq;
	int					_addrlen;
	

	uint8_t				_msg_buf[MSG_BUF_SIZE];

	
};
