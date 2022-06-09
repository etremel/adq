add_library(messaging OBJECT
    AggregationMessage.cpp
    OverlayMessage.cpp
    OverlayTransportMessage.cpp
    PathOverlayMessage.cpp
    QueryRequest.cpp
    SignatureRequest.cpp
    SignatureResponse.cpp)

target_include_directories(messaging PRIVATE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>)
target_compile_features(messaging PUBLIC cxx_std_17)
