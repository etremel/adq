#include <adq/core/QueryServer.hpp>
#include <adq/util/ConfigParser.hpp>

#include "SimParameters.hpp"
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

std::shared_ptr<adq::messaging::QueryRequest> make_measure_consumption_query(int query_num, int window_minutes) {
    // Construct a buffer containing the serialized arguments to the "measure consumption" function
    std::size_t select_args_size = mutils::bytes_size(window_minutes);
    std::vector<uint8_t> select_args(select_args_size);
    mutils::to_bytes(window_minutes, select_args.data());
    // The filter and aggregate functions don't have arguments, so create empty buffers for them
    std::vector<uint8_t> filter_args(0);
    std::vector<uint8_t> aggregate_args(0);
    return std::make_shared<adq::messaging::QueryRequest>(
        query_num, SimSmartMeter::SelectFunctions::MEASURE_CONSUMPTION,
        SimSmartMeter::FilterFunctions::NO_FILTER,
        SimSmartMeter::AggregateFunctions::SUM_VECTORS,
        select_args, filter_args, aggregate_args);
}

std::shared_ptr<adq::messaging::QueryRequest> make_shiftable_consumption_query(int query_num, int window_minutes) {
    // Construct a buffer containing the serialized arguments to the "measure shiftable consumption" function
    std::size_t select_args_size = mutils::bytes_size(window_minutes);
    std::vector<uint8_t> select_args(select_args_size);
    mutils::to_bytes(window_minutes, select_args.data());
    // The filter and aggregate functions don't have arguments, so create empty buffers for them
    std::vector<uint8_t> filter_args(0);
    std::vector<uint8_t> aggregate_args(0);
    return std::make_shared<adq::messaging::QueryRequest>(
        query_num, SimSmartMeter::SelectFunctions::MEASURE_SHIFTABLE_CONSUMPTION,
        SimSmartMeter::FilterFunctions::NO_FILTER,
        SimSmartMeter::AggregateFunctions::SUM_VECTORS,
        select_args, filter_args, aggregate_args);
}

}  // namespace smart_meters

int main(int argc, char** argv) {
    using namespace smart_meters;

    if(argc < 5) {
        std::cout << "Expected arguments: <port> <utility private key file> <meter IP configuration file> <public key folder>" << std::endl;
        return -1;
    }

    uint16_t service_port = std::stoi(argv[1]);
    std::string utility_private_key_file(argv[2]);

    // Read and parse the IP addresses
    std::map<int, asio::ip::tcp::endpoint> meter_ips_by_id = adq::util::read_ip_map_from_file(std::string(argv[3]));

    int num_meters = meter_ips_by_id.size();
    int modulus = adq::util::get_valid_prime_modulus(num_meters);
    if(modulus != num_meters) {
        std::cout << "ERROR: The number of meters specified in " << std::string(argv[3]) << " is not a valid prime. "
                  << "This experiment does not handle non-prime numbers of meters." << std::endl;
        return -1;
    }

    // Build the file paths for all the public keys based on the folder name
    std::string meter_public_key_folder = std::string(argv[4]);
    std::map<int, std::string> meter_public_key_files;
    for(int id = 0; id < num_meters; ++id) {
        std::stringstream file_path;
        file_path << meter_public_key_folder << "/pubkey_" << id << ".pem";
        meter_public_key_files.emplace(id, file_path.str());
    }

    adq::QueryServer<SimSmartMeter::DataRecordType> server(
        num_meters, service_port, meter_ips_by_id,
        utility_private_key_file, meter_public_key_files);

    server.register_query_callback(query_finished_callback);

    // Start a background thread to issue queries
    std::thread utility_query_thread([&server]() {
        for(int query_count = 0; query_count < NUM_QUERIES; ++query_count) {
            auto query_req = make_measure_consumption_query(query_count, 30);
            std::cout << "Starting query " << query_count << std::endl;
            server.start_query(query_req);
            int timesteps_in_30_min = 30 / USAGE_TIMESTEP_MIN;
            std::this_thread::sleep_for(TIME_PER_TIMESTEP * timesteps_in_30_min);
        }
        std::cout << "Done issuing queries" << std::endl;
        std::this_thread::sleep_for(TIME_PER_TIMESTEP * 3);
        server.shut_down();
    });
    //Start listening for incoming messages. This will block until the server shuts down.
    server.listen_loop();
    utility_query_thread.join();
}