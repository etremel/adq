#pragma once

#include "SimSmartMeter.hpp"

#include <map>
#include <string>

namespace smart_meters {

/**
 * Stores configuration information about the devices that can be used by a
 * simulated smart meter.
 */
struct DeviceConfig {
    /** The set of possible devices, indexed by name */
    std::map<std::string, Device> possible_devices;
    /** Maps a device name to the household saturation of that device, as a percentage. */
    std::map<std::string, double> devices_saturation;
    /**
     * Constructs a DeviceConfig object by loading device configuration data from files
     *
     * @param device_power_data_file A file that contains power usage data for each device by name
     * @param device_frequency_data_file A file that contains usage frequency data for each device by name
     * @param device_probability_data_file A file that contains hourly usage probabilities for each device by name
     * @param device_saturation_data_file A file that contains household saturation percentages for each device by name
     */
    DeviceConfig(const std::string& device_power_data_file, const std::string& device_frequency_data_file,
                 const std::string& device_probability_data_file, const std::string& device_saturation_data_file);

private:
    // Helper function that implements the parsing logic for the constructor
    void read_devices_from_files(const std::string& device_power_data_file, const std::string& device_frequency_data_file,
                                 const std::string& device_probability_data_file, const std::string& device_saturation_data_file);
};

}  // namespace smart_meters