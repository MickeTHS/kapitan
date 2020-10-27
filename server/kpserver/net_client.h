#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include <chrono>

#ifdef WIN32

#define NOMINMAX

#pragma comment(lib, "Ws2_32.lib")

#define WIN32_LEAN_AND_MEAN 1
#include <winsock2.h>
#include <winsock.h>
#include <windows.h>
#include <Ws2tcpip.h>
#else

#ifndef SOCKET
#define SOCKET int32_t
#endif


#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>

#endif

enum class NetClientType {
	Unauthenticated = 0,
	SlaveNode,
	Player
};


#define MSG_BUF_SIZE 20000

struct Net_client_info {
	Net_client_info() {
		ip = "";
		port = 0;
		tcp_socket = -1;
		udp_socket = -1;
		type = NetClientType::Unauthenticated;
		session_id = 0;
		client_id = 0;
		udp_code = 0;
		udp_port = 0;
		udp_established = false;
		quick_hash = 0;
	}

    std::string ip;
    int port;
	uint32_t int_ip;

	SOCKET tcp_socket;
	SOCKET udp_socket;
	NetClientType type;

	struct	sockaddr_in udp_addr;
	socklen_t           udp_addr_len;
	bool				udp_established;

	uint64_t quick_hash;
	uint32_t session_id;
	uint32_t client_id;
	uint16_t udp_code;
	uint16_t udp_port;

	void print() const {
		printf("[NET-CLIENT][fd: %d][p: %d][ip: %s]\n", socket, port, ip.c_str());
	}
};

struct Net_client {
	static uint32_t __ID_COUNTER;

    Net_client(Net_client_info info_, uint32_t id);
	virtual ~Net_client();

	void add_tcp_data(void* data, uint32_t len);
	void add_udp_data(void* data, uint32_t len);

	SOCKET get_tcp_socket() const;
	SOCKET get_udp_socket() const;

	void send_tcp_data();
	void send_udp_data();

	std::string get_ip() const;

    int mseconds_since_activity() const;

	void generate_udp_code();

	void print() const;

	uint32_t get_id() const;

	bool log_activity();
	
	Net_client_info     info;

private:

	std::vector<uint8_t> _tcp_data_buffer;
	std::vector<uint8_t> _udp_data_buffer;

	uint32_t _tcp_data_buffer_pos;
	uint32_t _udp_data_buffer_pos;

	uint32_t			_id;
	int					_num_flooded_packets;
	
	std::chrono::time_point<std::chrono::high_resolution_clock> _prev_activity;
};
