#pragma once

#include "../MessageBody.hpp"

#include "adq/messaging/AggregationMessageValue.hpp"
#include "adq/messaging/AgreementValue.hpp"
#include "adq/messaging/ByteBody.hpp"
#include "adq/messaging/MessageBodyType.hpp"
#include "adq/messaging/OverlayMessage.hpp"
#include "adq/messaging/PathOverlayMessage.hpp"
#include "adq/messaging/SignedValue.hpp"
#include "adq/messaging/ValueContribution.hpp"

namespace adq {
namespace messaging {

template <typename RecordType>
std::unique_ptr<MessageBody<RecordType>> MessageBody<RecordType>::from_bytes(mutils::DeserializationManager* m, uint8_t const* buffer) {
    // Reinterpret the first sizeof(MessageBodyType) bytes of buffer
    MessageBodyType body_type = *((MessageBodyType*)(buffer));
    // Dispatch to the correct subclass from_bytes based on the type
    switch(body_type) {
        case OverlayMessage<RecordType>::type:
            return OverlayMessage<RecordType>::from_bytes(m, buffer);
        case PathOverlayMessage<RecordType>::type:
            return PathOverlayMessage<RecordType>::from_bytes(m, buffer);
        case AggregationMessageValue<RecordType>::type:
            return AggregationMessageValue<RecordType>::from_bytes(m, buffer);
        case ValueContribution<RecordType>::type:
            return ValueContribution<RecordType>::from_bytes(m, buffer);
        case SignedValue<RecordType>::type:
            return SignedValue<RecordType>::from_bytes(m, buffer);
        case AgreementValue<RecordType>::type:
            return AgreementValue<RecordType>::from_bytes(m, buffer);
        case ByteBody<RecordType>::type:
            return ByteBody<RecordType>::from_bytes(m, buffer);
        default:
            assert(false && "Serialized MessageBody contained an invalid MessageBodyType!");
            return nullptr;
    }
}

}  // namespace messaging
}  // namespace adq