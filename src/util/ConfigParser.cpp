/**
 * @file ConfigParser.cpp
 *
 * @date Apr 13, 2017
 * @author edward
 */

#include <adq/util/ConfigParser.hpp>
#include <algorithm>
#include <asio.hpp>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <iterator>
#include <utility>
#include <vector>

using std::string;

namespace adq {
namespace util {

std::map<int, asio::ip::tcp::endpoint> read_ip_map_from_file(const std::string& meter_ips_file) {
    std::map<int, asio::ip::tcp::endpoint> meter_ips_by_id;

    std::ifstream ip_table_stream(meter_ips_file);
    string line;
    while(std::getline(ip_table_stream, line)) {
        std::istringstream id_ip_entry(line);
        int meter_id;
        std::string ip_address_string;
        int port_num;
        //ID and IP address are just separated by a space, so we can use the standard stream extractor
        id_ip_entry >> meter_id;
        id_ip_entry >> ip_address_string;
        id_ip_entry >> port_num;
        asio::ip::address ip_address = asio::ip::make_address(ip_address_string);
        meter_ips_by_id.emplace(meter_id, asio::ip::tcp::endpoint(ip_address, port_num));
    }

    return meter_ips_by_id;
}

}  // namespace util
}  // namespace adq
