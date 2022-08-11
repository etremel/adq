cmake_minimum_required(VERSION 3.16)
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
if(NOT CMAKE_BUILD_TYPE)
    message(WARNING "No build type selected! Compiler flags will probably be wrong.")
endif()

project(adq
    VERSION 1.0.0
    LANGUAGES C CXX)

    include(GNUInstallDirs)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -Wall -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")

find_package(mutils)
if (NOT ${mutils_FOUND})
    message(FATAL_ERROR "mutils not found. Please install from https://github.com/mpmilano/mutils.git")
endif()
find_package(spdlog 1.3.1 REQUIRED)
find_package(OpenSSL 1.1.1 REQUIRED)
# ASIO doesn't seem to export a CMake target, so we need to manually search for it
find_path(ASIO_PATH asio.hpp)
if(ASIO_PATH STREQUAL "ASIO_PATH-NOTFOUND")
    message(FATAL_ERROR "ASIO not found. You can download it from https://think-async.com/Asio")
endif()
# Is this global include *really* the best way to do it? Would it be better to add the dependency only to the targets that need it?
include_directories(${ASIO_PATH})

# The ConcurrentQueue library is just this one header
find_path(CONCURRENTQUEUE_PATH moodycamel/concurrentqueue.h)
if(CONCURRENTQUEUE_PATH STREQUAL "CONCURRENTQUEUE_PATH-NOTFOUND")
    message(FATAL_ERROR "ConcurrentQueue not found. Please install from https://github.com/cameron314/concurrentqueue")
endif()
include_directories(${CONCURRENTQUEUE_PATH})

add_subdirectory(src/core)
add_subdirectory(src/mutils-serialization)
add_subdirectory(src/messaging)
add_subdirectory(src/openssl)
add_subdirectory(src/util)
add_subdirectory(apps)

add_library(adq SHARED
    $<TARGET_OBJECTS:core>
    $<TARGET_OBJECTS:messaging>
    $<TARGET_OBJECTS:mutils-serialization>
    $<TARGET_OBJECTS:openssl_wrapper>
    $<TARGET_OBJECTS:util>)

target_link_libraries(adq
    pthread
    rt
    spdlog::spdlog
    ${mutils_LIBRARIES}
    OpenSSL::Crypto)

# When building (locally), the adq target has a public dependency on the local include directory
# This means the executable in the apps folder will know to look in the local include directory
# when they declare a dependency on the adq target. I don't think the target needs an include
# dependency for $<INSTALL_INTERFACE> because the install command already copies the include
# directory to the install location with install(DIRECTORY)
target_include_directories(adq PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/>)

target_compile_features(adq PUBLIC cxx_std_17)
set_target_properties(adq PROPERTIES
    SOVERSION ${adq_VERSION_MAJOR}.${adq_VERSION_MINOR}
    VERSION ${adq_VERSION}
    CXX_EXTENSIONS OFF)

# Setup for make install
# Declare that we will install the targets built by "adq"
install(TARGETS adq EXPORT adqTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
# Declare that we will install the "include/" directory as a standard include directory
install(DIRECTORY
        include/
        TYPE INCLUDE)

# Use CMakePackageConfigHelpers to create a package version file and config file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/adq/adqConfigVersion.cmake"
    VERSION ${adq_VERSION}
    COMPATIBILITY AnyNewerVersion
)

set(ConfigPackageLocation ${CMAKE_INSTALL_LIBDIR}/cmake/adq)

configure_package_config_file(${CMAKE_SOURCE_DIR}/cmake/adqConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/adq/adqConfig.cmake"
    INSTALL_DESTINATION ${ConfigPackageLocation}
    PATH_VARS CMAKE_INSTALL_LIBDIR CMAKE_INSTALL_INCLUDEDIR ConfigPackageLocation
)
