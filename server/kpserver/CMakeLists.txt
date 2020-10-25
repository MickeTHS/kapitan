cmake_minimum_required(VERSION 3.2)
project(kpserver)
set (CMAKE_BUILD_TYPE Release)

if(WIN32)
    add_definitions(-DWIN32)
else(WIN32)
    set(CMAKE_CXX_FLAGS "-std=c++11 -pthread -static-libstdc++ -O3")
    set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--no-undefined -pthread -std=c++11 -static-libstdc++ -O3")
endif(WIN32)

include_directories(ext)

set(KPSERVER_SRC_FILES
    main.cpp
    tcp_server.cpp
    tcp_server.h
    pos_socket.cpp
    pos_socket.h
    msg_conv.cpp
    msg_conv.h
    net_client.cpp
    net_client.h
    net_session.cpp
    net_session.h
    net_packet.cpp
    net_packet.h
    net_master.cpp
    net_master.h
    net_slave.cpp
    net_slave.h
    net_session_player.cpp
    net_session_player.h
    transform_entity.cpp
    transform_entity.h
    world_instance.cpp
    world_instance.h
    world_player_entity.cpp
    world_player_entity.h
    world_device_entity.cpp
    world_device_entity.h
    world_mission_entity.cpp
    world_mission_entity.h
    ini_file.cpp
    ini_file.h
    pos_session.h
    pos_session.cpp
    world_snapshot.h
    world_snapshot.cpp
    hash.cpp
    hash.h
    process_stats.h
    tcp_client.cpp
    tcp_client.h
)

if(WIN32)
    set(KPSERVER_SRC
        ${KPSERVER_SRC_FILES}
    )

elseif(APPLE)
    set(KPSERVER_SRC
        ${KPSERVER_SRC_FILES}
    )
elseif(UNIX)
    set(KPSERVER_SRC
        ${KPSERVER_SRC_FILES}
    )
endif()


add_executable(kpserver ${KPSERVER_SRC} )

if(WIN32)
    target_link_libraries(kpserver ws2_32 pdh )
elseif(APPLE)
    target_link_libraries(kpserver -static-libstdc++)
elseif(UNIX)
    target_link_libraries(kpserver dl -static-libstdc++)
endif()