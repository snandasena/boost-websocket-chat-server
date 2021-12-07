cmake_minimum_required(VERSION 3.19)
project(BoostChatServer)

set(CMAKE_CXX_STANDARD 17)

find_package(Boost COMPONENTS system REQUIRED)

file(GLOB SOURCE "${PROJECT_SOURCE_DIR}/src/*.cpp")

add_executable(BoostChatServer ${SOURCE})

target_link_libraries(BoostChatServer ${Boost_SYSTEM_LIBRARY})