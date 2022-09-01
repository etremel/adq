#pragma once

#include "DataSource.hpp"
#include "InternalTypes.hpp"
#include "MessageConsumer.hpp"
#include "NetworkManager.hpp"
#include "ProtocolState.hpp"

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
class QueryClient : public MessageConsumer<RecordType> {
public:
    /** The ID of this client device in the query network. */
    const int my_id;
    /** The total number of devices in the network. */
    int num_clients;

private:
    std::shared_ptr<spdlog::logger> logger;
    /** The NetworkManager object representing this client device's network interface. */
    NetworkManager<RecordType> network_manager;
    /** The ProtocolState object managing the query protocol for this client device. */
    ProtocolState<RecordType> query_protocol_state;
    /** The DataSource object that this device reads data from in response to a query. */
    std::unique_ptr<DataSource<RecordType>> data_source;

public:
    /**
     * Constructs a QueryClient by constructing all of its components. Different subsets
     * of the constructor arguments are used to construct different components. (In the
     * future, these options should be loaded from a configuration file instead of passed
     * as parameters).
     *
     * @param num_clients The total number of clients in the network
     * @param data_source The DataSource object that this client should read data
     * from in response to a query. The QueryClient takes ownership of this object.
     */
    QueryClient(int num_clients,
                std::unique_ptr<DataSource<RecordType>> data_source);

    virtual ~QueryClient() = default;

    /**
     * Handles an overlay message received from another client or the utility.
     * This is a callback for NetworkManager to invoke when a message arrives.
     * The QueryClient shares ownership of the message, so it will stay alive
     * until QueryClient is done handling it.
     */
    virtual void handle_message(std::shared_ptr<messaging::OverlayTransportMessage<RecordType>> message) override;
    /**
     * Handles an aggregation message received from another client. This is a
     * callback for NetworkManager to invoke when a message arrives.
     */
    virtual void handle_message(std::shared_ptr<messaging::AggregationMessage<RecordType>> message) override;
    /**
     * Handles a ping message received from another client. This is a callback
     * for NetworkManager to invoke when the message arrives.
     */
    virtual void handle_message(std::shared_ptr<messaging::PingMessage<RecordType>> message) override;
    /**
     * Starts the data collection protocol. This is a callback for NetworkManager
     * to invoke when a message arrives.
     * @param message The query request message received from the utility
     */
    virtual void handle_message(std::shared_ptr<messaging::QueryRequest<RecordType>> message) override;
    /**
     * Handles a signature-response message received from the utility. This is
     * a callback for NetworkManager to invoke when the message arrives.
     */
    virtual void handle_message(std::shared_ptr<messaging::SignatureResponse<RecordType>> message) override;
    /**
     * Handler for signature-request messages; they are ignored since only the
     * query server should receive signature-request messages.
     */
    virtual void handle_message(std::shared_ptr<messaging::SignatureRequest<RecordType>> message) override;

    /** Starts the client, which will continuously wait for messages and
     * respond to them as they arrive. This function call never returns. */
    void main_loop();

    int get_num_clients() const { return num_clients; }
};

}  // namespace adq

#include "detail/QueryClient_impl.hpp"
