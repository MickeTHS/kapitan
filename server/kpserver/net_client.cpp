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

Net_client::Net_client(Net_client_info info_, uint32_t id) 
	:	_num_flooded_packets(0),
		_tcp_data_buffer_pos(0),
		_udp_data_buffer_pos(0),
		_id(id),
		info(info_) {
	
	_tcp_data_buffer.resize(BUFFER_SIZE);
	_udp_data_buffer.resize(BUFFER_SIZE);
}

/**
* close all the sockets and servers, also delete the allocated pointers
*/
Net_client::~Net_client() {
	
#ifdef WIN32
	//WSACleanup();
	shutdown(info.tcp_socket, SD_BOTH);
	closesocket(info.tcp_socket);

	shutdown(info.udp_socket, SD_BOTH);
	closesocket(info.udp_socket);
#else
	close(_info.tcp_socket);
	close(_info.udp_socket);
#endif
}

std::string Net_client::get_ip() const {
	return info.ip;
}


void Net_client::add_tcp_data(void* data, uint32_t len) {
	memcpy(&_tcp_data_buffer[_tcp_data_buffer_pos], data, len);

	_tcp_data_buffer_pos += len;
}

void Net_client::add_udp_data(void* data, uint32_t len) {
	memcpy(&_udp_data_buffer[_udp_data_buffer_pos], data, len);

	_udp_data_buffer_pos += len;
}

void Net_client::send_tcp_data() {
	
	int result;
	uint32_t pos = 0;
	int32_t len = (int32_t)_tcp_data_buffer_pos;

	while (len > 0) {
		result = send(info.tcp_socket, (const char*)&_tcp_data_buffer[pos], len, 0);
		
		if (result == SOCKET_ERROR) {
			/*result = WSAGetLastError();

			if (result != WSAEWOULDBLOCK) {
				// ignore
			}
			*/
			_tcp_data_buffer_pos = 0;
			return;
		}
		else {
			len -= result;
			pos += result;
		}
	}

	_tcp_data_buffer_pos = 0;
}

void Net_client::send_udp_data() {
	if (_udp_data_buffer_pos == 0) { return; }

	send(info.udp_socket, (const char*)&_tcp_data_buffer[0], _udp_data_buffer_pos, 0);
	_udp_data_buffer_pos = 0;
}


uint32_t Net_client::get_id() const {
	return _id;
}

/// <summary>
/// Checks if the client is sending packets at a good rate
/// </summary>
/// <returns>true: good rate, false: packets are flooding</returns>
bool Net_client::log_activity() {
	
	auto now = std::chrono::high_resolution_clock::now();
	
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - _prev_activity).count();

	if (duration < 33) {
		_num_flooded_packets++;
	}

	_prev_activity = now;

	return _num_flooded_packets < 20;
}

int Net_client::mseconds_since_activity() const {
	auto now = std::chrono::high_resolution_clock::now();

	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - _prev_activity).count();

    return duration;
}

SOCKET Net_client::get_tcp_socket() const {
	return info.tcp_socket;
}

SOCKET Net_client::get_udp_socket() const {
	return info.udp_socket;
}

void Net_client::print() const {
	info.print();
}
