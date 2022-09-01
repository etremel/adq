add_library(config OBJECT Configuration.cpp)
target_include_directories(config PRIVATE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
)
target_compile_features(config PUBLIC cxx_std_17)
