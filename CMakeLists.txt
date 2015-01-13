cmake_minimum_required(VERSION 2.8)
project(s20)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
# set(CMAKE_AUTOMOC OFF)
SET(CMAKE_CXX_FLAGS "-std=gnu++11")

# Find the QtWidgets library
find_package(Qt5 REQUIRED Core Network)

# Tell CMake to create the helloworld executable
add_executable(s20 s20.cpp)

# Use the Widgets module from Qt 5.
target_link_libraries(s20 Qt5::Network)
