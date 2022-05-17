cmake_minimum_required(VERSION 3.16)
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
if(NOT CMAKE_BUILD_TYPE)
	message(WARNING "No build type selected! Compiler flags will probably be wrong.")
endif()

project(adq
	VERSION 1.0.0
	LANGUAGES CXX)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -Wall -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")

find_package(mutils)
if (NOT ${mutils_FOUND})
    message(FATAL_ERROR "mutils not found. Please install from https://github.com/mpmilano/mutils.git")
endif()
find_package(spdlog 1.3.1 REQUIRED)
find_package(OpenSSL 1.1.1 REQUIRED)
# find_package(ASIO 1.20.0 REQUIRED)
find_path(ASIO_PATH asio.hpp)
find_path(CONCURRENTQUEUE_PATH concurrentqueue.h)

add_subdirectory(src/core)
add_subdirectory(src/mutils-serialization)
add_subdirectory(src/messaging)
add_subdirectory(src/openssl)
add_subdirectory(src/util)
add_subdirectory(apps)

add_library(adq SHARED
	# $<TARGET_OBJECTS:core>
	$<TARGET_OBJECTS:mutils-serialization>
	$<TARGET_OBJECTS:messaging>
	$<TARGET_OBJECTS:openssl_wrapper>
	$<TARGET_OBJECTS:util>)

target_link_libraries(adq
	pthread
	rt
	${mutils_LIBRARIES}
	${OPENSSL_LIBRARIES})

target_compile_features(adq PUBLIC cxx_std_17)
set_target_properties(adq PROPERTIES
	SOVERSION ${adq_VERSION_MAJOR}.${adq_VERSION_MINOR}
	VERSION ${adq_VERSION}
	CXX_EXTENSIONS OFF)
