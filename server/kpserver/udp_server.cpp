#include "udp_server.h"

#include <stdio.h>
#include <string>
#include <stdio.h>
#include <string.h>

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
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

#endif


#include "net_packet.h"

Udp_server::Udp_server(int port)
	:	_port(port),
		_socket(-1),
		_on_data(nullptr)
	{

	// set up destination address 
	memset(&_addr, 0, sizeof(_addr));
	_addr.sin_family = AF_INET;
	_addr.sin_addr.s_addr = htonl(INADDR_ANY); // N.B.: differs from sender 
	_addr.sin_port = htons(_port);
	_addrlen = sizeof(_addr);

	_recv_buffer.resize(1024);
}

Udp_server::~Udp_server() {
#ifdef WIN32
	//WSACleanup();
	shutdown(_socket, SD_SEND);
	closesocket(_socket);
#else
	close(_socket);
#endif
}

void Udp_server::print_error() {
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

bool Udp_server::init() {
	printf("[POS-UDP][INIT]\n");

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
		printf("[POS-UDP][INIT][SOCKET][FAIL]\n");
		
#ifdef WIN32
		//WSACleanup();
#endif
		return false;
	}

#ifdef WIN32
	// allow multiple sockets to use the same PORT number 
	if (setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes)) < 0) {
		printf("[POS-UDP][INIT][REUSEADDR][FAIL]\n");

		//WSACleanup();

		return false;
	}
#else
	int enable = 1;
	if (setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
		printf("[POS-UDP][INIT][REUSEADDR][FAIL]\n");

		return false;
	}

#ifdef SO_REUSEPORT

	if (setsockopt(_socket, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int)) < 0) {
		printf("[POS-UDP][INIT][REUSEPORT][FAIL]\n");

		return false;
	}

#endif

#endif
	
	// bind to receive address 
	if (::bind(_socket, (struct sockaddr*)&_addr, sizeof(_addr)) < 0) {
		printf("[POS-UDP][INIT][BIND][FAIL]\n");
#ifdef WIN32
		//WSACleanup();
#endif
		return false;
	}

#ifdef WIN32
	unsigned long mode = 1;

	ioctlsocket(_socket, FIONBIO, &mode);
#else
	int status = fcntl(_socket, F_SETFL, fcntl(_socket, F_GETFL, 0) | O_NONBLOCK);

	if (status == -1) {
		printf("[POS-UDP][INIT][FIONBIO][FAIL]\n");
		// handle the error.  By the way, I've never seen fcntl fail in this way
	}

#endif

	printf("[POS-UDP][INIT][OK][p: %d]\n", _port);

    return true;
}

void Udp_server::set_on_data_callback(std::function<void(const std::vector<uint8_t>&, int32_t)> func) {
	_on_data = func;
}

int Udp_server::read() {
	
	int nbytes;

#ifdef WIN32
	while ((nbytes = recvfrom(_socket, (char*)&_recv_buffer[0], _recv_buffer.size(), 0, (struct sockaddr *) &_addr, &_addrlen)) >= 0) {
		printf("[POS-UDP][READ][l: %d][p: %d]\n", nbytes, ntohs(_addr.sin_port));

		if (_on_data != nullptr) {
			_on_data(_recv_buffer, nbytes);
		}
	}
#else
	while ((nbytes = recvfrom(_socket, (char*)&_recv_buffer[0], _recv_buffer.size(), 0, (struct sockaddr *) &_addr, (socklen_t*)&_addrlen) ) >= 0 ) {
		if (_on_data != nullptr) {
			_on_data(_recv_buffer, nbytes);
		}
	}
#endif

    return 0;
}

int Udp_server::send(const std::vector<uint8_t>& data) {

	return 0;
}

int Udp_server::get_port() const {
	return _port;
}