#pragma once

#include "InternalTypes.hpp"

#include <memory>

namespace adq {

/**
 * An interface for classes that can handle (consume) messages once they
 * have been deserialized by NetworkManager. This interface specifies that
 * the class must have a handle_message function for each type of message.
 * The handle_message functions share ownership of the message objects they
 * receive, in case the handler needs to store the message for later.
 *
 * @tparam RecordType The data type of a single record in the queries being
 * processed by this system. This is only needed to specify the template
 * parameter for AggregationMessages.
 */
template <typename RecordType>
class MessageConsumer {
public:
    /**
     * Handles an Overlay message received from a query client or server.
     */
    virtual void handle_message(std::shared_ptr<messaging::OverlayTransportMessage> message) = 0;
    /**
     * Handles an aggregation message received from a query client.
     */
    virtual void handle_message(std::shared_ptr<messaging::AggregationMessage<RecordType>> message) = 0;
    /**
     * Handles a ping message received from a query client.
     */
    virtual void handle_message(std::shared_ptr<messaging::PingMessage> message) = 0;
    /**
     * Handles a query request message from the query server (by starting the data collection protocol)
     */
    virtual void handle_message(std::shared_ptr<messaging::QueryRequest> message) = 0;
    /**
     * Handles a signature-response message received from the query server.
     */
    virtual void handle_message(std::shared_ptr<messaging::SignatureResponse> message) = 0;
    /**
     * Handles a signature-request message received from a query client.
     */
    virtual void handle_message(std::shared_ptr<messaging::SignatureRequest> message) = 0;

    virtual ~MessageConsumer();
};
}  // namespace adq