#pragma once

#include "adq/core/DataSource.hpp"
#include "adq/core/InternalTypes.hpp"
#include "adq/core/NetworkManager.hpp"
#include "adq/core/ProtocolState.hpp"

#include <spdlog/spdlog.h>

#include <asio.hpp>
#include <memory>

namespace adq {

/**
 * Represents a client in the anonymous distributed queries system running on a
 * single device (or process).
 *
 * @tparam RecordType The type of data for a single "record" (data point) that
 * this client can read from its encapsulated data source. All query operations
 * will read collections of this data type.
 */
template <typename RecordType>
class QueryClient {
public:
    /** The ID of this client device in the query network. */
    const int my_id;
    /** The total number of devices in the network. */
    int num_clients;

private:
    std::shared_ptr<spdlog::logger> logger;
    /** The ProtocolState object managing the query protocol for this client device. */
    ProtocolState<RecordType> query_protocol_state;
    /** The NetworkManager object representing this client device's network interface. */
    NetworkManager<RecordType> network_manager;
    /** The DataSource object that this device reads data from in response to a query. */
    std::unique_ptr<DataSource<RecordType>> data_source;

public:
    /**
     * Constructs a QueryClient by constructing all of its components. Different subsets
     * of the constructor arguments are used to construct different components. (In the
     * future, these options should be loaded from a configuration file instead of passed
     * as parameters).
     *
     * @param id The client's ID
     * @param num_clients The total number of clients in the network
     * @param service_port The port on which the client should listen for incoming
     * connections from other clients. Used to construct the NetworkManager.
     * @param client_id_to_ip_map A map from client IDs to IP addresses of other
     * clients in the network. Used to construct the NetworkManager.
     * @param private_key_filename The file containing the private key for this client.
     * Used to construct the CryptoLibrary.
     * @param public_key_files_by_id A map from client IDs to files containing the
     * public keys of those clients. Used to construct the CryptoLibrary.
     * @param data_source The DataSource object that this client should read data
     * from in response to a query. The QueryClient takes ownership of this object.
     */
    QueryClient(int id,
                int num_clients,
                uint16_t service_port,
                const std::map<int, asio::ip::tcp::endpoint>& client_id_to_ip_map,
                const std::string& private_key_filename,
                const std::map<int, std::string>& public_key_files_by_id,
                std::unique_ptr<DataSource<RecordType>> data_source);

    /**
     * Handles an overlay message received from another client or the utility.
     * This is a callback for NetworkManager to invoke when a message arrives.
     * The QueryClient shares ownership of the message, so it will stay alive
     * until QueryClient is done handling it.
     */
    void handle_message(std::shared_ptr<messaging::OverlayTransportMessage> message);
    /**
     * Handles an aggregation message received from another client. This is a
     * callback for NetworkManager to invoke when a message arrives.
     */
    void handle_message(std::shared_ptr<messaging::AggregationMessage<RecordType>> message);
    /**
     * Handles a ping message received from another client. This is a callback
     * for NetworkManager to invoke when the message arrives.
     */
    void handle_message(std::shared_ptr<messaging::PingMessage> message);
    /**
     * Starts the data collection protocol. This is a callback for NetworkManager
     * to invoke when a message arrives.
     * @param message The query request message received from the utility
     */
    void handle_message(std::shared_ptr<messaging::QueryRequest> message);
    /**
     * Handles a signature-response message received from the utility. This is
     * a callback for NetworkManager to invoke when the message arrives.
     */
    void handle_message(std::shared_ptr<messaging::SignatureResponse> message);

    /** Starts the client, which will continuously wait for messages and
     * respond to them as they arrive. This function call never returns. */
    void main_loop();

    /** Shuts down the message-listening loop to allow the client to exit cleanly.
     * Obviously, this must be called from a separate thread from main_loop(). */
    void shut_down();

    int get_num_clients() const { return num_clients; }
};

}  // namespace adq

#include "detail/QueryClient_impl.hpp"
