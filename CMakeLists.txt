cmake_minimum_required(VERSION 2.8)
project(s20)

add_definitions(-std=gnu++11)
set(CMAKE_AUTOMOC TRUE)

find_package(Qt5 REQUIRED Core Gui Widgets Network)

add_executable(s20 main.cpp socket.cpp consolereader.cpp server.cpp)
target_link_libraries(s20 Qt5::Core Qt5::Network)

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )
qt5_wrap_ui(UIS_HDRS dialog.ui)
add_executable(s20-gui main-gui.cpp dialog.cpp socket.cpp server.cpp ${UIS_HDRS})
target_link_libraries(s20-gui Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Network)
