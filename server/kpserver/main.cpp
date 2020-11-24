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

#include "net_session.h"
#include "world_instance.h"
#include "ini_file.h"
#include "net_master.h"
#include "net_slave.h"
#include "process_stats.h"
#include "trace.h"

// -ini C:\projects\kapitan_srv\server\build\kpserver\Debug\slave_eu1.ini
int main(int argc , char *argv[])
{
    TRACE("KPSERVER v0.1\n");

    char ini_filename[128];

    if (argc < 3) {
        TRACE("ERROR: Invalid argument count\n");
        
        TRACE("Usage: ./kpserver -ini master_eu.ini [-v]\n");
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
        TRACE("ERROR on startup: ini file missing is_me\n");
        return 0;
    }

    std::unique_ptr<Net_master> master_node = nullptr;
    std::unique_ptr<Net_slave> slave_node = nullptr;

    std::unique_ptr<Process_stats> stats = std::make_unique<Process_stats>();

    bool run = true;

    auto start_update = std::chrono::high_resolution_clock::now();

    int ticks_per_second = 30;
    uint64_t milliseconds_tickcount = (int)(1000.0f / (float)ticks_per_second);
    uint64_t duration = 0;
    int64_t idle_time = 0;

    if (node->is_master) {
        TRACE("[MAIN][MASTER][STARTUP]\n");

        master_node = std::make_unique<Net_master>(&ini);

        
    }
    else {
        TRACE("[MAIN][SLAVE][STARTUP]\n");

        // When the slave node is created, it will also create a udp socket
        slave_node = std::make_unique<Net_slave>(&ini, stats.get());
        
        if (!slave_node->validate_ini()) {
            TRACE("Quitting because of invalid ini file\n");
            return 0;
        }

        slave_node->setup_sessions();
        slave_node->print_sessions();

        // Initiate the slave node
        // This will connect to the master node
        // Also sends the slave node configuration to the master node
        if (!slave_node->init()) {
            TRACE("[SLAVE][MAIN][ERROR][Unable to initialize Net-Slave]\n");
            return 0;
        }
    }
    
    while (run) {
        start_update = std::chrono::high_resolution_clock::now();

        if (slave_node != nullptr) {
            slave_node->update();
        }
        else if (master_node != nullptr) {
            master_node->update();
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
