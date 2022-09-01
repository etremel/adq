# Anonymous Distributed Queries

This is the code repository for Anonymous Distributed Queries, a tool for querying sensitive data that resides in a large network of client devices, without collecting the data into a central location or revealing individual contributions. It includes a sample application that deploys the ADQ library on simulated smart power meters and uses it to collect their aggregate power usage data.

This library is a new and improved version of [PDDM](https://github.com/etremel/pddm) that does not attempt to include a network simulation. Clients and servers always communicate over a "real" network using ASIO, although they can of course be configured to run entirely on one machine by using local loopback addresses.

## Prerequisites

The ADQ library depends on the following libraries:

- OpenSSL
- [spdlog](https://github.com/gabime/spdlog)
- Non-Boost [ASIO](https://think-async.com/ASIO)
- [mutils](https://github.com/mpmilano/mutils)
- [ConcurrentQueue](https://github.com/cameron314/concurrentqueue)

## Configuration

Each client and server must have a config.ini file that provides various system configuration parameters, and this must be loaded by calling adq::Configuration::initialize() before constructing a client or server instance. Sample configuration files are provided in the src/config directory.

The Smart Meters application has additional configuration parameters that it expects to be added to config.ini. A sample configuration file for the Smart Meters application is included in the apps/smart_meters directory.
