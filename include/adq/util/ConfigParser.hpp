#pragma once

#include <asio.hpp>
#include <map>
#include <string>

namespace adq {

namespace util {

/**
 * Parses a configuration file that contains a whitespace-separated table of
 * device IDs, IP addresses, and ports, and returns the corresponding map of
 * device ID to TCP endpoint.
 */
std::map<int, asio::ip::tcp::endpoint> read_ip_map_from_file(const std::string& meter_ips_file);

} /* namespace util */
}  // namespace adq
