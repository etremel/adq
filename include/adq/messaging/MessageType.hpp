#pragma once

#include <cstdint>
#include <iostream>

namespace adq {
namespace messaging {

enum class MessageType : int16_t {
    OVERLAY, /* Indicates an OverlayTransportMessage, not an OverlayMessage, since OverlayMessages will always be "wrapped" */
    PING,
    AGGREGATION,
    QUERY_REQUEST,
    SIGNATURE_REQUEST,
    SIGNATURE_RESPONSE
};

inline std::ostream& operator<<(std::ostream& stream, const MessageType& value) {
    switch(value) {
    case MessageType::OVERLAY:
        return stream << "OVERLAY";
    case MessageType::PING:
        return stream << "PING";
    case MessageType::AGGREGATION:
        return stream << "AGGREGATION";
    case MessageType::QUERY_REQUEST:
        return stream << "QUERY_REQUEST";
    case MessageType::SIGNATURE_REQUEST:
        return stream << "SIGNATURE_REQUEST";
    case MessageType::SIGNATURE_RESPONSE:
        return stream << "SIGNATURE_RESPONSE";
    default:
        return stream << "UNKNOWN";
    }
}

}
}


