#include "adq/config/Configuration.hpp"
#include "adq/config/inifile-cpp/inicpp.h"

#include <asio.hpp>
#include <atomic>
#include <cstdint>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

namespace adq {

const std::string Configuration::DEFAULT_CONFIG_FILE = "config.ini";
// This static variable must be re-declared here even though it is not initialized here
std::unique_ptr<Configuration> Configuration::instance;

const std::string Configuration::SECTION_SETUP = "Setup";
const std::string Configuration::CLIENT_ID = "client_id";
const std::string Configuration::CLIENT_PORT = "client_port";
const std::string Configuration::SERVER_PORT = "server_port";
const std::string Configuration::PRIVATE_KEY_FILE = "private_key_file";
const std::string Configuration::SERVER_PUBLIC_KEY_FILE = "server_key_file";
const std::string Configuration::CLIENT_LIST_FILE = "client_list_file";
const std::string Configuration::CLIENT_KEYS_FOLDER = "client_keys_folder";
const std::string Configuration::CLIENT_KEY_FILE_PREFIX = "client_key_file_prefix";

std::atomic<int> Configuration::initialize_state = 0;

Configuration::Configuration(const std::string& config_file_path) {
    parsed_config_file.setMultiLineValues(true);
    parsed_config_file.load(config_file_path);
    // Sanity test config file
    if(parsed_config_file.find(SECTION_SETUP) == parsed_config_file.end()) {
        throw std::logic_error("Configuration file error: Required section [" + SECTION_SETUP + "] not found");
    }
    if(!hasKey(SECTION_SETUP, CLIENT_PORT)) {
        throw std::logic_error("Configuration file error: Required key " + CLIENT_PORT + " not found");
    }
    if(!hasKey(SECTION_SETUP, SERVER_PORT)) {
        throw std::logic_error("Configuration file error: Required key " + SERVER_PORT + " not found");
    }
    if(!hasKey(SECTION_SETUP, PRIVATE_KEY_FILE)) {
        throw std::logic_error("Configuration file error: Required key " + PRIVATE_KEY_FILE + " not found");
    }
}

Configuration& Configuration::getInstance() {
    while(Configuration::initialize_state.load(std::memory_order_acquire) != STATE_INITIALIZED) {
        Configuration::initialize(DEFAULT_CONFIG_FILE);
    }
    return *instance;
}

void Configuration::initialize(const std::string& config_file_path) {
    int expected_uninitialized = STATE_UNINITIALIZED;
    if(initialize_state.compare_exchange_strong(
           expected_uninitialized, STATE_INITIALIZING, std::memory_order_acq_rel)) {
        // make_unique doesn't work if the constructor is private
        instance = std::unique_ptr<Configuration>(new Configuration(config_file_path));
        initialize_state.store(STATE_INITIALIZED, std::memory_order_acq_rel);
    }
    // make sure concurrent callers only return when initialization has finished
    while(initialize_state.load(std::memory_order_acquire) != STATE_INITIALIZED) {
    }
}

bool Configuration::hasKey(const std::string& section, const std::string& key) const {
    return parsed_config_file.at(section).find(key) != parsed_config_file.at(section).end();
}

std::int16_t Configuration::getInt16(const std::string& section, const std::string& key) {
    return getInstance().get<std::int16_t>(section, key);
}

std::int32_t Configuration::getInt32(const std::string& section, const std::string& key) {
    return getInstance().get<std::int32_t>(section, key);
}

std::int64_t Configuration::getInt64(const std::string& section, const std::string& key) {
    return getInstance().get<std::int64_t>(section, key);
}

std::uint16_t Configuration::getUInt16(const std::string& section, const std::string& key) {
    return getInstance().get<std::uint16_t>(section, key);
}

std::uint32_t Configuration::getUInt32(const std::string& section, const std::string& key) {
    return getInstance().get<std::uint32_t>(section, key);
}

std::uint64_t Configuration::getUInt64(const std::string& section, const std::string& key) {
    return getInstance().get<std::uint64_t>(section, key);
}

std::string Configuration::getString(const std::string& section, const std::string& key) {
    return getInstance().get<std::string>(section, key);
}

bool Configuration::getBool(const std::string& section, const std::string& key) {
    return getInstance().get<bool>(section, key);
}

double Configuration::getDouble(const std::string& section, const std::string& key) {
    return getInstance().get<double>(section, key);
}

float Configuration::getFloat(const std::string& section, const std::string& key) {
    return getInstance().get<float>(section, key);
}

char Configuration::getChar(const std::string& section, const std::string& key) {
    return getInstance().get<char>(section, key);
}

std::map<int, asio::ip::tcp::endpoint> read_ip_map_from_file(const std::string& client_list_file) {
    std::map<int, asio::ip::tcp::endpoint> meter_ips_by_id;

    std::ifstream ip_table_stream(client_list_file);
    std::string line;
    while(std::getline(ip_table_stream, line)) {
        std::istringstream id_ip_entry(line);
        int meter_id;
        std::string ip_address_string;
        int port_num;
        // ID and IP address are just separated by a space, so we can use the standard stream extractor
        id_ip_entry >> meter_id;
        id_ip_entry >> ip_address_string;
        id_ip_entry >> port_num;
        asio::ip::address ip_address = asio::ip::make_address(ip_address_string);
        meter_ips_by_id.emplace(meter_id, asio::ip::tcp::endpoint(ip_address, port_num));
    }

    return meter_ips_by_id;
}

std::map<int, std::string> make_client_key_paths(const std::string& client_keys_folder, int num_clients) {
    std::map<int, std::string> client_public_key_files;
    for(int id = 0; id < num_clients; ++id) {
        std::stringstream file_path;
        file_path << client_keys_folder
                  << Configuration::getString(Configuration::SECTION_SETUP, Configuration::CLIENT_KEY_FILE_PREFIX)
                  << id << ".pem";
        client_public_key_files.emplace(id, file_path.str());
    }
    return client_public_key_files;
}
}  // namespace adq
