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

#include "tcp_server.h"
#include "udp_server.h"
#include "net_session.h"
#include "world_instance.h"
#include "ini_file.h"
#include "net_master.h"
#include "net_slave.h"
#include "process_stats.h"


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

    auto node = ini.get_me();

    if (node == nullptr) {
        printf("ERROR on startup: ini file missing is_me\n");
        return 0;
    }

    Tcp_server tcp(node->tcp_port);

    std::vector<std::shared_ptr<Net_session>> sessions;

    std::shared_ptr<Net_master> master_node = nullptr;
    std::shared_ptr<Net_slave> slave_node = nullptr;

    std::shared_ptr<Process_stats> stats = std::make_shared<Process_stats>();

    if (node->is_master) {
        printf("[MAIN][MASTER][STARTUP]\n");

        // master nodes doesnt start groups

        master_node = std::make_shared<Net_master>(&tcp, ini);
    }
    else {
        printf("[MAIN][SLAVE][STARTUP]\n");

        slave_node = std::make_shared<Net_slave>(&tcp, ini, stats);
        slave_node->setup_sessions();

    }
    
    tcp.init();
    
    bool run = true;

    auto start_update = std::chrono::high_resolution_clock::now();

    int ticks_per_second = 30;
    uint64_t milliseconds_tickcount = (int)(1000.0f / (float)ticks_per_second);
    uint64_t duration = 0;
    int64_t idle_time = 0;

    uint64_t test = 0;

    if (master_node != nullptr) {
        //if (master_node->init)
    }
    else if (slave_node != nullptr) {
        if (!slave_node->init()) {
            printf("[SLAVE][MAIN][ERROR][Unable to initialize Net-Slave]\n");
            return 0;
        }
    }

    while (run) {
        start_update = std::chrono::high_resolution_clock::now();

        tcp.read();

        for (auto session : sessions) {
            session->read();
        }

        if (slave_node != nullptr) {
            slave_node->update();
        }
        else if (master_node != nullptr) {
            master_node->update();
        }

        for (uint64_t i = 0; i < 1000000; ++i) {
            test = ((test * 44) * test) / 10000;
        }
        
        auto now = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_update).count();
        idle_time = milliseconds_tickcount - duration;

        // the entire tick cycle took duration milliseconds
        // we want a tick to only fire every milliseconds_tickcount, so we need to sleep the rest of the time
        
        stats->add_tick_idle_timing(idle_time);

        if (idle_time > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(idle_time));
            stats->is_overloaded = false;
        }
        else {
            stats->is_overloaded = true;
        }
    }

    return 0;
}
