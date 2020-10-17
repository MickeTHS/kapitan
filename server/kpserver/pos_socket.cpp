#include "pos_socket.h"

#include <stdio.h>
#include <string>
#include <stdio.h>

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

Pos_socket::Pos_socket(int port) {
    _port = port;
	_socket = -1;

	printf("POS Socket created with port: %d\n", port);
	
	// set up destination address 
	memset(&_addr, 0, sizeof(_addr));
	_addr.sin_family = AF_INET;
	_addr.sin_addr.s_addr = htonl(INADDR_ANY); // N.B.: differs from sender 
	_addr.sin_port = htons(_port);
	_addrlen = sizeof(_addr);

	_recv_buffer.resize(1024);
}

Pos_socket::~Pos_socket() {
#ifdef WIN32
	WSACleanup();
	shutdown(_socket, SD_SEND);
	closesocket(_socket);
#else
	close(_info.socket);
#endif
}

bool Pos_socket::init() {
	int	 nbytes, addrlen;

	uint8_t yes = 1;

#ifdef WIN32
	WSADATA wsaData;

	int wsa_result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (wsa_result != 0) {
		printf("WSAStartup failed with error: %d\n", wsa_result);
		return false;
	}
#endif
	// create what looks like an ordinary UDP socket
	if ((_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("Unable to open socket\n");
		perror("socket");
#ifdef WIN32
		WSACleanup();
#endif
		return false;
	}

#ifdef WIN32
	// allow multiple sockets to use the same PORT number 
	if (setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes)) < 0) {
		printf("Error reusing address\n");
		perror("Reusing ADDR failed");

		WSACleanup();

		return false;
	}
#else
	int enable = 1;
	if (setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
		perror("setsockopt(SO_REUSEADDR) failed");

		return false;
	}

#ifdef SO_REUSEPORT

	if (setsockopt(_socket, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int)) < 0) {
		perror("setsockopt(SO_REUSEPORT) failed");

		return false;
	}

#endif

#endif

	
	// bind to receive address 
	if (::bind(_socket, (struct sockaddr*)&_addr, sizeof(_addr)) < 0) {
		printf("Error on bind\n");
		perror("bind");
#ifdef WIN32
		WSACleanup();
#endif
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

	printf("upd socket started on port %d\n", _port);

    return true;
}

int Pos_socket::read() {
	
	int nbytes;

#ifdef WIN32
	while ((nbytes = recvfrom(_socket, (char*)&_recv_buffer[0], _recv_buffer.size(), 0, (struct sockaddr *) &_addr, &_addrlen)) >= 0) {
		
	}
#else
	while ((nbytes = recvfrom(_socket, (char*)&_recv_buffer[0], _recv_buffer.size(), 0, (struct sockaddr *) &_addr, (socklen_t*)&_addrlen) ) >= 0 ) {
		handle_rec_packet(&_msg_buf[0], nbytes);
	}
#endif

    return 0;
}

int Pos_socket::send(const std::vector<uint8_t>& data) {

	return 0;
}

int Pos_socket::get_port() const {
	return _port;
}