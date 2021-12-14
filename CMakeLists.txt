cmake_minimum_required(VERSION 3.10)
project(BoostChatServer)

set(CMAKE_CXX_STANDARD 17)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pthread -pedantic")

find_package(Boost COMPONENTS system REQUIRED)


file(GLOB SOURCE
        src/beast.h
        src/http_session.cpp
        src/http_session.h
        src/listener.cpp
        src/listener.h
        src/main.cpp
        src/net.h
        src/shared_state.cpp
        src/shared_state.h
        src/websocket_session.cpp
        src/websocket_session.h)

add_executable(BoostChatServer ${SOURCE} ${BOOST_BEAST_FILES})

target_link_libraries(BoostChatServer ${Boost_SYSTEM_LIBRARY})