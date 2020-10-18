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

#include <chrono>

#include "con_socket.h"
#include "pos_socket.h"
#include "net_group.h"

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

    int con_port = 0;
    
    char listen_addr[64];
    
    if (argc < 3) {
        printf("ERROR: Invalid argument count\n");
        // -a <listen address> , removed, listen on ANY
        printf("Usage: ./kpserver -pcon <port controller> -pchat <port chat> -ppos <port positions>  [-v]\n");
        return 0;
    }

    //parse the arguments
    for (int i = 1; i < argc; ++i) {
    	
    	if(!strcmp(argv[i],"-pcon")) {
    		con_port = atoi(argv[i+1]);
    	}

        // we dont need -a
    	if(!strcmp(argv[i],"-a")) {
            strcpy(listen_addr, argv[i+1]);
		}
    }

    if (con_port <= 0) {
        printf("ERROR: controller port: %d\n", con_port);
        return 0;
    }

    Con_socket tcp(con_port);
    Net_group group0(123, 10, &tcp);


    tcp.set_on_new_client_callback([&](std::shared_ptr<Net_client> client){
        group0.add_client(client);
        group0.send_config();
    });

    tcp.init();
    

    char buffer[2000];
    char sendbuffer[2000];

    // POS socket must be accepted later, we dont know when it will connect
    //std::thread tsockets(&Listen_sockets::run, &tcp);
    
    bool run = true;

    // bug? always registered as open, weird
    while (run) {
        tcp.read(con_port);
        group0.read();

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
