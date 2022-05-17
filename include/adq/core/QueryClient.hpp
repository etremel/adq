#pragma once

#include "adq/core/DataSource.hpp"
#include "adq/core/InternalTypes.hpp"
#include "adq/core/NetworkManager.hpp"
#include "adq/core/ProtocolState.hpp"

#include <spdlog/spdlog.h>

#include <asio.hpp>
#include <memory>

namespace adq {

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
    QueryClient(int id, int num_clients, std::unique_ptr<DataSource<RecordType>> data_source);

    /** Handles a message received from another meter or the utility.
     * This is a callback for NetworkManager to invoke when a message arrives. */
    void handle_message(const std::shared_ptr<messaging::OverlayTransportMessage>& message);
    /** @copydoc handle_message(const std::shared_ptr<messaging::OverlayTransportMessage>&) */
    void handle_message(const std::shared_ptr<messaging::AggregationMessage>& message);
    /** @copydoc handle_message(const std::shared_ptr<messaging::OverlayTransportMessage>&) */
    void handle_message(const std::shared_ptr<messaging::PingMessage>& message);
    /** @copydoc handle_message(const std::shared_ptr<messaging::OverlayTransportMessage>&) */
    void handle_message(const std::shared_ptr<messaging::QueryRequest>& message);
    /** @copydoc handle_message(const std::shared_ptr<messaging::OverlayTransportMessage>&) */
    void handle_message(const std::shared_ptr<messaging::SignatureResponse>& message);

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
