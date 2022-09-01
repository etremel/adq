#include <adq/config/Configuration.hpp>
#include <adq/core/QueryServer.hpp>

#include "SimProperties.hpp"
#include "SimSmartMeter.hpp"

#include <iostream>
#include <memory>
#include <thread>
#include <vector>

namespace smart_meters {

void query_finished_callback(const int query_num,
                             std::shared_ptr<adq::messaging::AggregationMessageValue<SimSmartMeter::DataRecordType>> result) {
    std::cout << "Query " << query_num << " finished." << std::endl;
    std::cout << "Result was: " << *result << std::endl;
}

std::shared_ptr<adq::messaging::QueryRequest<SimSmartMeter::DataRecordType>> make_measure_consumption_query(int query_num, int window_minutes) {
    // Construct a buffer containing the serialized arguments to the "measure consumption" function
    std::size_t select_args_size = mutils::bytes_size(window_minutes);
    std::vector<uint8_t> select_args(select_args_size);
    mutils::to_bytes(window_minutes, select_args.data());
    // The filter and aggregate functions don't have arguments, so create empty buffers for them
    std::vector<uint8_t> filter_args(0);
    std::vector<uint8_t> aggregate_args(0);
    return std::make_shared<adq::messaging::QueryRequest<SimSmartMeter::DataRecordType>>(
        query_num, SimSmartMeter::SelectFunctions::MEASURE_CONSUMPTION,
        SimSmartMeter::FilterFunctions::NO_FILTER,
        SimSmartMeter::AggregateFunctions::SUM_VECTORS,
        select_args, filter_args, aggregate_args);
}

std::shared_ptr<adq::messaging::QueryRequest<SimSmartMeter::DataRecordType>> make_shiftable_consumption_query(int query_num, int window_minutes) {
    // Construct a buffer containing the serialized arguments to the "measure shiftable consumption" function
    std::size_t select_args_size = mutils::bytes_size(window_minutes);
    std::vector<uint8_t> select_args(select_args_size);
    mutils::to_bytes(window_minutes, select_args.data());
    // The filter and aggregate functions don't have arguments, so create empty buffers for them
    std::vector<uint8_t> filter_args(0);
    std::vector<uint8_t> aggregate_args(0);
    return std::make_shared<adq::messaging::QueryRequest<SimSmartMeter::DataRecordType>>(
        query_num, SimSmartMeter::SelectFunctions::MEASURE_SHIFTABLE_CONSUMPTION,
        SimSmartMeter::FilterFunctions::NO_FILTER,
        SimSmartMeter::AggregateFunctions::SUM_VECTORS,
        select_args, filter_args, aggregate_args);
}

}  // namespace smart_meters

int main(int argc, char** argv) {
    using namespace smart_meters;

    std::string config_file;
    if(argc > 2) {
        config_file = argv[1];
    } else {
        config_file = adq::Configuration::DEFAULT_CONFIG_FILE;
    }
    // Load configuration options
    adq::Configuration::initialize(config_file);

    // Read the list of clients to determine how many there are
    std::map<int, asio::ip::tcp::endpoint> meter_ips_by_id = adq::read_ip_map_from_file(
        adq::Configuration::getString(adq::Configuration::SECTION_SETUP, adq::Configuration::CLIENT_LIST_FILE));

    int num_meters = meter_ips_by_id.size();
    int modulus = adq::util::get_valid_prime_modulus(num_meters);
    if(modulus != num_meters) {
        std::cout << "ERROR: The number of meters specified in " << std::string(argv[3]) << " is not a valid prime. "
                  << "This experiment does not handle non-prime numbers of meters." << std::endl;
        return -1;
    }

    adq::QueryServer<SimSmartMeter::DataRecordType> server(num_meters);

    server.register_query_callback(query_finished_callback);

    // Start a background thread to issue queries
    std::thread utility_query_thread([&server]() {
        const int num_queries = adq::Configuration::getInt32(SECTION_SIMULATION, NUM_QUERIES);
        const int timestep_min = adq::Configuration::getInt32(SECTION_SIMULATION, USAGE_TIMESTEP_MIN);
        const auto time_per_timestep = std::chrono::milliseconds(
            adq::Configuration::getInt32(SECTION_SIMULATION, MS_PER_TIMESTEP));
        for(int query_count = 0; query_count < num_queries; ++query_count) {
            auto query_req = make_measure_consumption_query(query_count, 30);
            std::cout << "Starting query " << query_count << std::endl;
            server.start_query(query_req);
            int timesteps_in_30_min = 30 / timestep_min;
            std::this_thread::sleep_for(time_per_timestep * timesteps_in_30_min);
        }
        std::cout << "Done issuing queries" << std::endl;
        std::this_thread::sleep_for(time_per_timestep * 3);
        server.shut_down();
    });
    // Start listening for incoming messages. This will block until the server shuts down.
    server.listen_loop();
    utility_query_thread.join();
}
