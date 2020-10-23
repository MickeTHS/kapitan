cmake_minimum_required(VERSION 3.2)
PROJECT(kpserver)
SET (CMAKE_BUILD_TYPE Release)

IF(WIN32)
add_definitions(-DWIN32)
set(CMAKE_CXX_FLAGS "-m32 -msse -std=c++11 -pthread")
ELSE(WIN32)
set(CMAKE_CXX_FLAGS "-std=c++11 -pthread -static-libstdc++ -O3")
set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--no-undefined -pthread -std=c++11 -static-libstdc++ -O3")
ENDIF(WIN32)

include_directories(ext)

SET(KPSERVER_SRC_FILES
    main.cpp
    con_socket.cpp
    con_socket.h
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
)

if(WIN32)
SET(KPSERVER_SRC
${KPSERVER_SRC_FILES}
)
elseif(APPLE)
SET(KPSERVER_SRC
${KPSERVER_SRC_FILES}
)
elseif(UNIX)
SET(KPSERVER_SRC
${KPSERVER_SRC_FILES}
)
endif()


ADD_EXECUTABLE(kpserver
${KPSERVER_SRC}
)

if(WIN32)
TARGET_LINK_LIBRARIES(kpserver ws2_32 )

elseif(APPLE)

target_link_libraries(kpserver -static-libstdc++)

elseif(UNIX)

TARGET_LINK_LIBRARIES(kpserver dl -static-libstdc++)
endif()