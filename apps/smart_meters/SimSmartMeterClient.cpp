#include <adq/core/QueryClient.hpp>
#include <adq/util/ConfigParser.hpp>

#include <regex>
#include <string>
#include <thread>

#include "DeviceConfig.hpp"
#include "SimParameters.hpp"
#include "SimSmartMeter.hpp"

namespace smart_meters {

inline bool device_already_picked(const std::list<Device>& existing_devices, const std::regex& name_pattern) {
    for(const auto& existing_device : existing_devices) {
        if(std::regex_search(existing_device.name, name_pattern))
            return true;
    }
    return false;
}

/**
 * Factory that creates a new simulated meter by picking a set of devices at
 * random, based on an income distribution and the devices' configuration options.
 * @param income_distribution The percentages of low, middle, and high-income
 * homes in the region being simulated
 * @param config The DeviceConfig containing a set of devices and their saturations
 * @param random_engine A source of randomness. Specified to be std::mt19937
 * because (bizarrely) there's no common supertype for randomness engines, and
 * this is the one I happen to use in Simulator.
 * @return A new SimSmartMeter with a random set of devices
 */
std::unique_ptr<SimSmartMeter> generate_meter(std::discrete_distribution<>& income_distribution,
                                              const DeviceConfig& config,
                                              std::mt19937& random_engine) {
    int income_choice = income_distribution(random_engine);
    IncomeLevel income_level = income_choice == 0 ? IncomeLevel::POOR : (income_choice == 1 ? IncomeLevel::AVERAGE : IncomeLevel::RICH);
    // Pick what devices this home owns based on their saturation percentages
    std::list<Device> home_devices;
    for(const auto& device_saturation : config.devices_saturation) {
        // Devices that end in digits have multiple "versions," and only one of them should be in home_devices
        std::regex ends_in_digits(".*[0-9]$", std::regex::extended);
        if(std::regex_match(device_saturation.first, ends_in_digits)) {
            std::regex name_prefix(device_saturation.first.substr(0, device_saturation.first.length() - 2) + std::string(".*"), std::regex::extended);
            if(device_already_picked(home_devices, name_prefix)) {
                continue;
            }
        }
        // Homes have either a window or central AC but not both
        std::regex conditioner("conditioner");
        if(std::regex_search(device_saturation.first, conditioner) && device_already_picked(home_devices, conditioner)) {
            continue;
        }
        // Otherwise, randomly decide whether to include this device, based on its saturation
        double saturation_as_fraction = device_saturation.second / 100.0;
        if(std::bernoulli_distribution(saturation_as_fraction)(random_engine)) {
            home_devices.emplace_back(config.possible_devices.at(device_saturation.first));
        }
    }
    return std::make_unique<SimSmartMeter>(income_level, home_devices);
}

}  // namespace smart_meters

int main(int argc, char** argv) {
    using namespace smart_meters;

    if(argc < 8) {
        std::cout << "Expected arguments: <my ID> <utility IP address> <meter IP configuration file> <device configuration files> " << std::endl;
        std::cout << "Device characteristic files are: power load, mean daily frequency, "
                     "hourly usage probability, and household saturation."
                  << std::endl;
        return -1;
    }

    // First argument is my ID
    int meter_id = std::atoi(argv[1]);

    // Second argument is assumed to be the utility's IP:port
    auto utility_ip_string = std::string(argv[2]);
    auto colon_pos = utility_ip_string.find_first_of(':');
    asio::ip::tcp::endpoint utility_ip(asio::ip::make_address(utility_ip_string.substr(0, colon_pos)), std::stoi(utility_ip_string.substr(colon_pos + 1)));

    // Read and parse the IP addresses
    std::map<int, asio::ip::tcp::endpoint> meter_ips_by_id = adq::util::read_ip_map_from_file(std::string(argv[3]));

    int num_meters = meter_ips_by_id.size();
    int modulus = adq::util::get_valid_prime_modulus(num_meters);
    if(modulus != num_meters) {
        std::cout << "ERROR: The number of meters specified in " << std::string(argv[3]) << " is not a valid prime. "
                  << "This experiment does not handle non-prime numbers of meters." << std::endl;
        return -1;
    }

    // Read and parse the device configurations
    DeviceConfig device_config{std::string(argv[4]), std::string(argv[5]),
                               std::string(argv[6]), std::string(argv[7])};
    std::mt19937 random_engine;
    std::discrete_distribution<> income_distribution({25, 50, 25});
    // Generate a meter with random devices
    std::unique_ptr<SimSmartMeter> sim_meter = generate_meter(income_distribution, device_config, random_engine);
    // Start the meter's background thread
    sim_meter->run_simulation();
    adq::QueryClient<std::vector<adq::FixedPoint_t>> client(meter_id, num_meters, std::move(sim_meter));
    // Start waiting for incoming messages to respond to. This will not return.
    client.main_loop();

    return 0;
}