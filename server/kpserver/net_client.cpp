#include "net_client.h"
#include <stdio.h>
#include <string>
#include <stdio.h>

#ifndef WIN32
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#define BUFFER_SIZE 2000

uint32_t Net_client::__ID_COUNTER = 0;

Net_client::Net_client(Net_client_info info, uint32_t id) {
	_info = info;
	_id = id;

	send_buffer.resize(BUFFER_SIZE);
	buffer_pos = 0;
}

/**
* close all the sockets and servers, also delete the allocated pointers
*/
Net_client::~Net_client() {
	
#ifdef WIN32
	WSACleanup();
	shutdown(_info.socket, SD_SEND);
	closesocket(_info.socket);
#else
	close(_info.socket);
#endif
}

std::string Net_client::get_ip() const {
	return _info.ip;
}

bool Net_client::request_buffer_size(uint32_t size) const {
	if (buffer_pos + size < BUFFER_SIZE) {
		return true;
	}

	return false;
}

void Net_client::clear_buffer() {
	buffer_pos = 0;
}

uint32_t Net_client::get_id() const {
	return _id;
}

bool Net_client::init() {
	
	/*int	 nbytes, addrlen;
	
    uint8_t yes = 1;

#ifdef WIN32
	WSADATA wsaData;

	int wsa_result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (wsa_result != 0) {
		printf("WSAStartup failed with error: %d\n", wsa_result);
		return 1;
	}
#endif
	// create what looks like an ordinary UDP socket
	if ((_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
#ifdef WIN32
		WSACleanup();
#endif
		return false;
	}


#ifdef WIN32
    // allow multiple sockets to use the same PORT number 
    if (setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes)) < 0) {
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

	// set up destination address 
	memset(&_addr, 0, sizeof(_addr));
	_addr.sin_family = AF_INET;
	_addr.sin_addr.s_addr = htonl(INADDR_ANY); // N.B.: differs from sender 
	_addr.sin_port = htons(_info.port);

	// bind to receive address 
	if (::bind(_socket, (struct sockaddr *) &_addr, sizeof(_addr)) < 0) {
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

	if (status == -1){
	    perror("calling fcntl");
	    // handle the error.  By the way, I've never seen fcntl fail in this way
	}

#endif

	printf("upd socket started on port %d\n", _port);
	*/
	return true;
}

void Net_client::handle_rec_packet(uint8_t* data, int msglen) {
	/*int offset = 0;
	int len = msglen;

	uint8_t* buffer = data;

	while (len > 0) {
		int type = _msg_conv->convert_msg(&buffer[offset], len, msglen);

		if (type == E_MsgError) {
            printf("ERROR: Message resulted in error\n");
            break;
        }

        _receiver->recv_msg(_msg_conv->_msg_all, type);

		offset += msglen;
		len -= msglen;
	}
	*/
}

int Net_client::mseconds_since_activity() const {
    return 0;
}

SOCKET Net_client::get_socket() const {
	return _info.socket;
}

void Net_client::print() const {
	_info.print();
}

void Net_client::print_byte_buffer(uint8_t *buffer, int length) {
	std::string debugOut;

	for (int i = 0; i < length; ++i) {
		debugOut += " " + std::to_string((uint8_t)buffer[i]);
	}

	//printf("%s\n", debugOut.c_str());
}

/* call this in infinite loop */
void Net_client::read() {
    /*int nbytes;

    _addrlen = sizeof(_addr);
	
#ifdef WIN32
    while ((nbytes = recvfrom(_socket, (char*)&_msg_buf[0], MSG_BUF_SIZE, 0,
			(struct sockaddr *) &_addr, &_addrlen)) >= 0) {
		handle_rec_packet(&_msg_buf[0], nbytes);
    }
#else

    while ( (nbytes = recvfrom(_socket, (char*)&_msg_buf[0], MSG_BUF_SIZE, 0, (struct sockaddr *) &_addr, (socklen_t*)&_addrlen) ) >= 0 ) {
        handle_rec_packet(&_msg_buf[0], nbytes);
    }
#endif*/
}
