#pragma once

#include <spdlog/spdlog.h>
#include <asio.hpp>

#include <adq/core/InternalTypes.hpp>
#include <adq/core/ProtocolState.hpp>
#include <adq/core/NetworkManager.hpp>
#include <adq/core/DataSource.hpp>
#include <memory>

namespace adq {

template<typename RecordType>
class QueryClient {
public:
    /** The ID of this client device in the query network. */
    const int my_id;
    /** The total number of devices in the network. */
    int num_clients;

private:
    std::shared_ptr<spdlog::logger> logger;
    ProtocolState query_protocol_state;
    NetworkManager network_manager;
    std::unique_ptr<DataSource<RecordType>> data_source;

public:
    QueryClient(int id, int num_clients, std::unique_ptr<DataSource<RecordType>> data_source);

    /** Handles a message received from another meter or the utility.
     * This is a callback for NetworkClient to invoke when a message arrives from the network_client. */
    void handle_message(const std::shared_ptr<messaging::OverlayTransportMessage>& message);
    /** @copydoc handle_message(const std::shared_ptr<messaging::OverlayTransportMessage>&) */
    void handle_message(const std::shared_ptr<messaging::AggregationMessage>& message);
    /** @copydoc handle_message(const std::shared_ptr<messaging::OverlayTransportMessage>&) */
    void handle_message(const std::shared_ptr<messaging::PingMessage>& message);
    /** @copydoc handle_message(const std::shared_ptr<messaging::OverlayTransportMessage>&) */
    void handle_message(const std::shared_ptr<messaging::QueryRequest>& message);
    /** @copydoc handle_message(const std::shared_ptr<messaging::OverlayTransportMessage>&) */
    void handle_message(const std::shared_ptr<messaging::SignatureResponse>& message);
};

}  // namespace adq

#include "detail/QueryClient_impl.hpp"
