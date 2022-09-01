include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(SimSmartMeterClient SimSmartMeterClient.cpp SimSmartMeter.cpp DeviceConfig.cpp SimProperties.cpp)
target_link_libraries(SimSmartMeterClient adq)
target_compile_features(SimSmartMeterClient PUBLIC cxx_std_17)

add_executable(UtilityServer UtilityServer.cpp SimProperties.cpp)
target_link_libraries(UtilityServer adq)
target_compile_features(UtilityServer PUBLIC cxx_std_17)
