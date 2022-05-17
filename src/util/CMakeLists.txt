cmake_minimum_required(VERSION 3.16)
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

add_library(util OBJECT
    ConfigParser.cpp Overlay.cpp PathFinder.cpp
    LinuxTimerManager.cpp)

target_include_directories(util PRIVATE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>)
target_compile_features(util PUBLIC cxx_std_17)
