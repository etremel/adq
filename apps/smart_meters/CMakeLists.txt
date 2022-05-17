cmake_minimum_required(VERSION 3.16)
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(SimSmartMeterClient SimSmartMeterClient.cpp SimSmartMeter.cpp)
target_link_libraries(SimSmartMeterClient adq)
target_compile_features(SimSmartMeterClient PUBLIC cxx_std_17)