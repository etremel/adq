#pragma once

#include "inifile-cpp/inicpp.h"

#include <asio.hpp>
#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <string>

namespace adq {

/**
 * A singleton class that manages system configuration options, which are
 * loaded from an INI file. This must be initialized before any QueryServer
 * or QueryClient object is created.
 */
class Configuration {
private:
    /** The single instance of this class */
    static std::unique_ptr<Configuration> instance;
    /** Atomic flag indicating the status of the initialize() function, used to make it thread-safe */
    static std::atomic<int> initialize_state;
    static constexpr int STATE_UNINITIALIZED = 0;
    static constexpr int STATE_INITIALIZING = 1;
    static constexpr int STATE_INITIALIZED = 2;
    /** The IniFile object containing the properties parsed from a file */
    ini::IniFile parsed_config_file;
    /** Private constructor called by the initialize() method */
    Configuration(const std::string& config_file_path);

public:
    /**
     * Gets a reference to the singleton configuration object. If initialize()
     * has not yet been called, this will first initialize the object with the
     * default configuration file.
     */
    static Configuration& getInstance();
    /**
     * Initializes the system configuration by reading a file from the provided path.
     *
     * @param config_file_path The path to the INI configuration file.
     */
    static void initialize(const std::string& config_file_path);
    /**
     * Checks to see if the loaded configuration has a key with the given name.
     * Call this before calling a get() method to ensure it will succeed.
     * @param section The name of the configuration file section in which
     * the key should be found
     * @param key The name of the key
     * @return true If a key with this name was loaded from the INI file, false
     * if it was not.
     */
    bool hasKey(const std::string& section, const std::string& key) const;

    /**
     * Retrieves a configuration property of a specified type. This is just
     * a wrapper around the accessor methods of the inifile-cpp library.
     *
     * @tparam T The datatype of the property. This should be a built-in numeric
     * type, or std::string, otherwise conversion to this type may fail.
     * @param section The section in which the property is found
     * @param key The name of the property
     * @return The property's value, as loaded from the config file
     */
    template <typename T>
    T get(const std::string& section, const std::string& key) {
        return parsed_config_file[section][key].as<T>();
    }
    /**
     * The name of the configuration file that will be loaded by default
     * if get() is called without first calling initialize().
     */
    static const std::string DEFAULT_CONFIG_FILE;

    // Convenience methods to get a property of a particular type from the singleton instance
    static std::string getString(const std::string& section, const std::string& key);
    static std::uint16_t getUInt16(const std::string& section, const std::string& key);
    static std::uint32_t getUInt32(const std::string& section, const std::string& key);
    static std::uint64_t getUInt64(const std::string& section, const std::string& key);
    static std::int16_t getInt16(const std::string& section, const std::string& key);
    static std::int32_t getInt32(const std::string& section, const std::string& key);
    static std::int64_t getInt64(const std::string& section, const std::string& key);
    static float getFloat(const std::string& section, const std::string& key);
    static double getDouble(const std::string& section, const std::string& key);
    static bool getBool(const std::string& section, const std::string& key);
    static char getChar(const std::string& section, const std::string& key);

    // Right now, all config options will be in one section called "setup"
    static const std::string SECTION_SETUP;
    // String constants for the different configuration keys expected by this program
    /** The unique ID for the current running client. Not needed by the server. */
    static const std::string CLIENT_ID;
    /** The port on which clients listen for incoming messages (from each other or the server) */
    static const std::string CLIENT_PORT;
    /** The port on which the server listens for incoming messages */
    static const std::string SERVER_PORT;
    /** The path to a file containing the private key for the running program (client or sever) */
    static const std::string PRIVATE_KEY_FILE;
    /** The path to a file containing the public key for the query server */
    static const std::string SERVER_PUBLIC_KEY_FILE;
    /** The path to a file listing the ID numbers and IP addresses of all the clients in the system */
    static const std::string CLIENT_LIST_FILE;
    /** The path to a folder containing public keys for all of the clients, named by their client IDS. */
    static const std::string CLIENT_KEYS_FOLDER;
    /**
     * The prefix for the names of files containing client public keys; the
     * client's ID and the extension ".pem" will be appended to this prefix
     * when searching for a client's key.
     */
    static const std::string CLIENT_KEY_FILE_PREFIX;
};

// Some stand-alone utility methods that help construct configuration data from the properties

/**
 * Parses a client-list file that contains a whitespace-separated table of
 * device IDs, IP addresses, and ports, and returns the corresponding map of
 * device ID to TCP endpoint.
 */
std::map<int, asio::ip::tcp::endpoint> read_ip_map_from_file(const std::string& client_list_file);

/**
 * Constructs a path to the (expected) public key file of each client, given
 * a base path (to the folder the keys should be in) and the total number of
 * clients. Uses the configured CLIENT_KEY_FILE_PREFIX as the prefix for each
 * file name.
 *
 * @return A map from device ID to the file containing that device's public key,
 * including the (relative) directory path
 */
std::map<int, std::string> make_client_key_paths(const std::string& client_keys_folder, int num_clients);

}  // namespace adq
