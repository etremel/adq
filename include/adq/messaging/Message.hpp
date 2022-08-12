#pragma once

#include "MessageBody.hpp"
#include "adq/mutils-serialization/SerializationSupport.hpp"

#include <memory>

namespace adq {

/** The "node ID" that will be used to refer to the utility/system owner in
 *  messages. The utility is not a node that participates in the overlay, but
 *  message-passing functions still need to send messages to the utility. */
const int UTILITY_NODE_ID = -1;

namespace messaging {

/**
 * Base class for all messages sent between devices in this system.
 * Defines the header fields that are common to all messages.
 *
 * @tparam RecordType The data type being collected by queries in this
 * instantiation of the query system; needed because some types of
 * message body will contain instances of this data type.
 */
template <typename RecordType>
class Message : public mutils::ByteRepresentable {
public:
    int sender_id;
    std::shared_ptr<MessageBody<RecordType>> body;
    Message(const int sender_id, std::shared_ptr<MessageBody<RecordType>> body) : sender_id(sender_id), body(std::move(body)){};
    virtual ~Message() = default;
    // Common serialization code for the superclass header fields.
    // These functions DO NOT include a MessageType field, which must be added by a subclass.
    std::size_t bytes_size() const;
    std::size_t to_bytes(uint8_t* buffer) const;
    void post_object(const std::function<void(uint8_t const* const, std::size_t)>&) const;

    // Calls a subclass from_bytes
    static std::unique_ptr<Message<RecordType>> from_bytes(mutils::DeserializationManager* m, uint8_t const* buffer);
};

}  // namespace messaging

}  // namespace adq

#include "detail/Message_impl.hpp"