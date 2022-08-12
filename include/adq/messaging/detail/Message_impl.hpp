#pragma once

#include "../Message.hpp"

#include "adq/messaging/AggregationMessage.hpp"
#include "adq/messaging/MessageType.hpp"
#include "adq/messaging/OverlayTransportMessage.hpp"
#include "adq/messaging/PingMessage.hpp"
#include "adq/messaging/QueryRequest.hpp"
#include "adq/messaging/SignatureRequest.hpp"
#include "adq/messaging/SignatureResponse.hpp"
#include "adq/mutils-serialization/SerializationSupport.hpp"

namespace adq {
namespace messaging {

template <typename RecordType>
std::size_t Message<RecordType>::bytes_size() const {
    return mutils::bytes_size(sender_id) + mutils::bytes_size(*body);
}

template <typename RecordType>
std::size_t Message<RecordType>::to_bytes(uint8_t* buffer) const {
    std::size_t bytes_written = mutils::to_bytes(sender_id, buffer);
    // body is not optional, so there's no need to check if it's null
    //(the only subclass that doesn't use body also overrides this method)
    bytes_written += mutils::to_bytes(*body, buffer + bytes_written);
    return bytes_written;
}

template <typename RecordType>
void Message<RecordType>::post_object(const std::function<void(const uint8_t* const, std::size_t)>& function) const {
    mutils::post_object(function, sender_id);
    mutils::post_object(function, *body);
}

template <typename RecordType>
std::unique_ptr<Message<RecordType>> Message<RecordType>::from_bytes(mutils::DeserializationManager* m, uint8_t const* buffer) {
    // Peek at the first sizeof(MessageType) bytes of buffer to determine the MessageType
    MessageType message_type = *((MessageType*)(buffer));
    // Dispatch to the correct subclass from_bytes based on the type
    switch(message_type) {
        case OverlayTransportMessage<RecordType>::type:
            return OverlayTransportMessage<RecordType>::from_bytes(m, buffer);
        case PingMessage<RecordType>::type:
            return PingMessage<RecordType>::from_bytes(m, buffer);
        case AggregationMessage<RecordType>::type:
            return AggregationMessage<RecordType>::from_bytes(m, buffer);
        case QueryRequest<RecordType>::type:
            return QueryRequest<RecordType>::from_bytes(m, buffer);
        case SignatureRequest<RecordType>::type:
            return SignatureRequest<RecordType>::from_bytes(m, buffer);
        case SignatureResponse<RecordType>::type:
            return SignatureResponse<RecordType>::from_bytes(m, buffer);
        default:
            return nullptr;
    }
}

}  // namespace messaging
}  // namespace adq