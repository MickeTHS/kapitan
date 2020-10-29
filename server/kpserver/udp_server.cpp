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

#include "net_session_player.h"
#include "net_packet.h"

#include "trace.h"

Udp_server::Udp_server()
	:	_port(0),
		_socket(-1),
		_on_client_data(nullptr),
		_on_client_connect(nullptr)
	{
	// set up destination address 
	memset(&_addr, 0, sizeof(_addr));
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
	TRACE("%S\n", s);
	LocalFree(s);
#else
	TRACE("Error: %s\n", strerror(errno));
#endif
}

bool Udp_server::init(int port) {
	TRACE("[POS-UDP][INIT]\n");
	_addr.sin_family = AF_INET;
	_addr.sin_addr.s_addr = htonl(INADDR_ANY); // N.B.: differs from sender 
	_addr.sin_port = htons(_port);
	
	int	 nbytes, addrlen;

	uint8_t yes = 1;

#ifdef WIN32
	WSADATA wsaData;

	int wsa_result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (wsa_result != 0) {
		TRACE("WSAStartup failed with error: %d\n", wsa_result);
		return false;
	}
#endif
	// create what looks like an ordinary UDP socket
	if ((_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		TRACE("[POS-UDP][INIT][SOCKET][FAIL]\n");
		
#ifdef WIN32
		//WSACleanup();
#endif
		return false;
	}

#ifdef WIN32
	// allow multiple sockets to use the same PORT number 
	if (setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes)) < 0) {
		TRACE("[POS-UDP][INIT][REUSEADDR][FAIL]\n");

		//WSACleanup();

		return false;
	}
#else
	int enable = 1;
	if (setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
		TRACE("[POS-UDP][INIT][REUSEADDR][FAIL]\n");

		return false;
	}

#ifdef SO_REUSEPORT

	if (setsockopt(_socket, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int)) < 0) {
		TRACE("[POS-UDP][INIT][REUSEPORT][FAIL]\n");

		return false;
	}

#endif

#endif
	
	// bind to receive address 
	if (::bind(_socket, (struct sockaddr*)&_addr, sizeof(_addr)) < 0) {
		TRACE("[POS-UDP][INIT][BIND][FAIL]\n");
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
		TRACE("[POS-UDP][INIT][FIONBIO][FAIL]\n");
		// handle the error.  By the way, I've never seen fcntl fail in this way
	}

#endif

	TRACE("[POS-UDP][INIT][OK][p: %d]\n", _port);

    return true;
}

/// <summary>
/// We need to keep track of which remote port+address corresponds to the incoming port
/// in order to correctly map the packets from a client
/// The TCP socket must first signal the client to send a Net_Udp_establish message
/// In this package we send a code that the client must use in order to 
/// properly map the client
/// </summary>
/// <param name="client"></param>
void Udp_server::establish_client_connection(Net_client* client) {
	client->generate_udp_code();

	_client_id_map[client->info.client_id] = client;
}

void Udp_server::set_on_client_data_callback(std::function<void(Net_client*, const std::vector<uint8_t>&, int32_t)> func) {
	_on_client_data = func;
}

void Udp_server::set_on_client_connect_callback(std::function<void(Net_client*, const Net_Udp_establish&)> func) {
	_on_client_connect = func;
}

int Udp_server::read() {
	struct sockaddr_in client_addr;
	int client_addr_len = sizeof(client_addr);

	memset(&client_addr, 0, sizeof(client_addr));

	int nbytes;

#ifdef WIN32
	while ((nbytes = recvfrom(_socket, (char*)&_recv_buffer[0], _recv_buffer.size(), 0, (struct sockaddr *) &client_addr, &client_addr_len)) >= 0) {
#else 
	while ((nbytes = recvfrom(_socket, (char*)&_recv_buffer[0], _recv_buffer.size(), 0, (struct sockaddr*) &client_addr, (socklen_t*)&client_addr_len)) >= 0) {
#endif
		MsgType type = (MsgType)((uint8_t)_recv_buffer[0]);

		if (type == MsgType::NetUDPEstablish) {
			// This is the very first message that the client has sent to the server over UDP
			// We previously requested that the client send this message via our TCP server
			// using a Net_Udp_client_connection_info packet, where we sent the client code and
			// client id
			Net_Udp_establish est(_recv_buffer, 0);
			
			uint16_t port = ntohs(client_addr.sin_port);
			uint32_t ip = (uint32_t)ntohl(client_addr.sin_addr.s_addr);

			// fill the IP and port in a wider integer
			uint64_t quick_hash = ((uint64_t)ip << 16) + port;

			// fetch the client based on client_id
			if (_client_id_map.find(est.client_id) == _client_id_map.end()) {
				// we didnt find a client, so this is weird
				return 0;
			}

			auto client = _client_id_map[est.client_id];
			
			// make sure the code matches
			if (client->info.udp_code != est.code) {
				TRACE("Code mismatch\n");
				return 0;
			}

			// assign the address structs
			memcpy(&client->info.udp_addr, &client_addr, sizeof(client_addr));
			client->info.udp_addr_len = client_addr_len;
			client->info.udp_established = true;
			client->info.quick_hash = quick_hash;

			// assign the client in our IP+Port client map for fast lookups
			_client_hash_map[quick_hash] = client;

			// notifiy our listeners on the new UDP client connection
			if (_on_client_connect != nullptr) {
				_on_client_connect(_client_hash_map[quick_hash], est);
			}

			return 0;
		}

		uint16_t port = ntohs(client_addr.sin_port);
		uint32_t ip = (uint32_t)ntohl(client_addr.sin_addr.s_addr);

		// find the client by IP+Port
		uint64_t quick_hash = ((uint64_t)ip << 16) + port;

		if (_client_hash_map.find(quick_hash) != _client_hash_map.end()) {
			// notify our data listeners
			if (_on_client_data != nullptr) {
				_on_client_data(_client_hash_map[quick_hash], _recv_buffer, nbytes);
			}
		}
	}

    return 0;
}

/// <summary>
/// Make sure we remove the client in a clean way
/// </summary>
/// <param name="client"></param>
void Udp_server::remove_client(Net_client* client) {
	if (_client_hash_map.find(client->info.quick_hash) != _client_hash_map.end()) {
		_client_hash_map.erase(client->info.quick_hash);
	}

	if (_client_id_map.find(client->info.client_id) != _client_id_map.end()) {
		_client_id_map.erase(client->info.client_id);
	}
}

/// <summary>
/// When we want to send out data to all players within a session
/// </summary>
/// <param name="players"></param>
/// <param name="num_players"></param>
/// <param name="data"></param>
/// <param name="data_len"></param>
/// <returns></returns>
int Udp_server::session_send(const std::vector<Net_session_player*>& players, int num_players, const std::vector<uint8_t>& data, uint32_t data_len) {
	Net_session_player* player = 0;

	for (int i = 0; i < num_players; ++i) {
		player = players[i];
		
		sendto(_socket, (const char*)&data[0], data_len, 0, (const sockaddr*)&(player->client_connection->info.udp_addr), player->client_connection->info.udp_addr_len);
	}

	return 0;
}

/// <summary>
/// When we want to send out data to clients in a given vector (perhaps to keep things secret from someone?)
/// </summary>
/// <param name="clients"></param>
/// <param name="data"></param>
/// <param name="data_len"></param>
/// <returns></returns>
int Udp_server::send(const std::vector<Net_client*>& clients, const std::vector<uint8_t>& data, uint32_t data_len) {
	
	for (auto client : clients) {
		if (!client->info.udp_established) {
			continue;
		}

		sendto(_socket, (const char*)&data[0], data_len, 0, (const sockaddr *)&client->info.udp_addr, client->info.udp_addr_len);
	}

	return 0;
}

int Udp_server::get_port() const {
	return _port;
}