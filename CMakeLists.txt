cmake_minimum_required(VERSION 2.8)
project(s20)

add_definitions(-std=gnu++11)
set(CMAKE_AUTOMOC TRUE)

find_package(Qt5 REQUIRED Core Network)
add_executable(s20 main.cpp socket.cpp consolereader.cpp server.cpp)
target_link_libraries(s20 Qt5::Core Qt5::Network)
