add_library(util OBJECT
    Overlay.cpp
    PathFinder.cpp
    LinuxTimerManager.cpp)

target_include_directories(util PRIVATE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>)
target_compile_features(util PUBLIC cxx_std_17)
