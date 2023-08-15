add_library(core OBJECT CryptoLibrary.cpp)

target_include_directories(core PRIVATE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>)
target_compile_features(core PUBLIC cxx_std_17)
target_link_libraries(core mutils::mutils)
