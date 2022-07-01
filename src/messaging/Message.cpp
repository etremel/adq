#include "adq/messaging/Message.hpp"

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

std::size_t Message::bytes_size() const {
     return mutils::bytes_size(sender_id) + mutils::bytes_size(*body);
}

std::size_t Message::to_bytes(uint8_t* buffer) const {
    std::size_t bytes_written = mutils::to_bytes(sender_id, buffer);
    //body is not optional, so there's no need to check if it's null
    //(the only subclass that doesn't use body also overrides this method)
    bytes_written += mutils::to_bytes(*body, buffer + bytes_written);
    return bytes_written;
}

void Message::post_object(const std::function<void(const uint8_t* const, std::size_t)>& function) const {
    mutils::post_object(function, sender_id);
    mutils::post_object(function, *body);
}

std::unique_ptr<Message> Message::from_bytes(mutils::DeserializationManager* m, uint8_t const * buffer) {
    //Peek at the first sizeof(MessageType) bytes of buffer to determine the MessageType
    MessageType message_type = *((MessageType*)(buffer));
    //Dispatch to the correct subclass from_bytes based on the type
    switch(message_type) {
    case OverlayTransportMessage::type:
        return OverlayTransportMessage::from_bytes(m, buffer);
    case PingMessage::type:
        return PingMessage::from_bytes(m, buffer);
    // Oh no! This switch no longer works for AggregationMessage because it has a template parameter
    // I think nothing actually uses Message::from_bytes, though, so maybe I can just get rid of it
    // case AggregationMessage::type:
        // return AggregationMessage::from_bytes(m, buffer);
    case QueryRequest::type:
        return QueryRequest::from_bytes(m, buffer);
    case SignatureRequest::type:
        return SignatureRequest::from_bytes(m, buffer);
    case SignatureResponse::type:
        return SignatureResponse::from_bytes(m, buffer);
    default:
        return nullptr;
    }
}

}
}