#include <stdio.h>
#include <string.h>    //strlen


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
#include <unistd.h>    //write
#endif

#include <stdlib.h>
#include <fcntl.h>
#include <thread>
#include <string>
#include <memory>

#include <chrono>

#include "con_socket.h"
#include "pos_socket.h"
#include "net_session.h"
#include "world_instance.h"
#include "ini_file.h"
#include "net_master.h"

void print_byte_buffer(uint8_t *buffer, int length) {
	std::string debugOut;

	for (int i = 0; i < length; ++i) {
		debugOut += " " + std::to_string((uint8_t)buffer[i]);
	}

	printf("%s\n", debugOut.c_str());
}

int main(int argc , char *argv[])
{
    printf("KPSERVER v0.1\n");

    char ini_filename[128];

    if (argc < 3) {
        printf("ERROR: Invalid argument count\n");
        
        printf("Usage: ./kpserver -ini master_eu.ini [-v]\n");
        return 0;
    }

    //parse the arguments
    for (int i = 1; i < argc; ++i) {
    	
    	if (!strcmp(argv[i], "-ini")) {
            strcpy(ini_filename, argv[i + 1]);
        }
    }

    Ini_file ini(ini_filename);
    ini.read();

    if (ini.node == nullptr) {
        printf("ERROR on startup: ini file missing [MASTER] or [SLAVE]\n");
        return 0;
    }

    Con_socket tcp(ini.node->port);

    std::vector<std::shared_ptr<Net_session>> sessions;

    if (ini.node->is_master) {
        printf("[MAIN][MASTER][STARTUP]\n");

        // master nodes doesnt start groups

        std::shared_ptr<Net_master> master = std::make_shared<Net_master>(&tcp);
    }
    else {
        printf("[MAIN][SLAVE][STARTUP]\n");

        for (int i = 0; i < ini.node->max_groups; ++i) {

            std::shared_ptr<Net_session> session = std::make_shared<Net_session>(ini.node->id + (1 + i), ini.node->max_users_per_group, &tcp, ini.node->udp_range_min + i);
            sessions.push_back(session);

            tcp.set_on_new_client_callback([&](std::shared_ptr<Net_client> client) {
                session->add_client(client);
                session->send_config();
            });
        }
    }
    
    tcp.init();
    
    bool run = true;

    while (run) {
        tcp.read(ini.node->port);

        for (auto session : sessions) {
            session->read();
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        /*
        auto start = std::chrono::high_resolution_clock::now(); 

        int source = -1;
        int len = -1;

        // listen for CON messages, always
        if ((source = tcp.listen_socket(tcp.con_socket(), buffer, 2000, len)) != -1) {

            memset(sendbuffer, 0, 2000);
            memcpy(sendbuffer, buffer, len);
            
        }

        // only listen for POS if we have established POS connection
        if ((source = tcp.listen_socket(tcp.pos_socket(), buffer, 2000, len)) != -1) {
            // do we need to reply to sim server??
            // forward to vis computers
        }

        // make sure that the execution cycle is fixed around 10000 microseconds
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
        int sleep_ms = 10000 - duration.count();
        
        if (sleep_ms > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
        }*/
    }

    return 0;
}
