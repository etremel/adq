#include <adq/core/QueryClient.hpp>
#include <adq/util/ConfigParser.hpp>

#include "SimSmartMeter.hpp"

int main(int argc, char** argv) {
    using namespace adq;

    if(argc < 8) {
        std::cout << "Expected arguments: <my ID> <utility IP address> <meter IP configuration file> <device configuration files> " << std::endl;
        std::cout << "Device characteristic files are: power load, mean daily frequency, "
                "hourly usage probability, and household saturation." << std::endl;
        return -1;
    }

    //First argument is my ID
    int meter_id = std::atoi(argv[1]);

    //Second argument is assumed to be the utility's IP:port
    auto utility_ip_string = std::string(argv[2]);
    auto colon_pos = utility_ip_string.find_first_of(':');
    asio::ip::tcp::endpoint utility_ip(asio::ip::make_address(utility_ip_string.substr(0, colon_pos)), std::stoi(utility_ip_string.substr(colon_pos + 1)));

    //Read and parse the IP addresses
    std::map<int, asio::ip::tcp::endpoint> meter_ips_by_id = util::read_ip_map_from_file(std::string(argv[3]));

    int num_meters = meter_ips_by_id.size();
    int modulus = util::get_valid_prime_modulus(num_meters);
    if(modulus != num_meters) {
        std::cout << "ERROR: The number of meters specified in " << std::string(argv[3]) << " is not a valid prime. "
                << "This experiment does not handle non-prime numbers of meters." << std::endl;
        return -1;
    }
    std::unique_ptr<SimSmartMeter> sim_meter = std::make_unique<SimSmartMeter>();
    adq::QueryClient<std::vector<adq::FixedPoint_t>> client(meter_id, num_meters, std::move(sim_meter));
    client.main_loop();
}