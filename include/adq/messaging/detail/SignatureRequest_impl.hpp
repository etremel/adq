#pragma once

#include "../SignatureRequest.hpp"

#include "adq/messaging/Message.hpp"
#include "adq/messaging/MessageType.hpp"
#include "adq/mutils-serialization/SerializationSupport.hpp"

#include <cstdint>
#include <cstring>
#include <memory>
#include <ostream>

namespace adq {
namespace messaging {

template<typename RecordType>
const constexpr MessageType SignatureRequest<RecordType>::type;

template<typename RecordType>
std::size_t SignatureRequest<RecordType>::bytes_size() const {
    return mutils::bytes_size(type) + Message<RecordType>::bytes_size();
}
template<typename RecordType>
std::size_t SignatureRequest<RecordType>::to_bytes(uint8_t* buffer) const {
    std::size_t bytes_written = mutils::to_bytes(type, buffer);
    bytes_written += Message<RecordType>::to_bytes(buffer + bytes_written);
    return bytes_written;
}
template<typename RecordType>
void SignatureRequest<RecordType>::post_object(const std::function<void(const uint8_t* const, std::size_t)>& function) const {
    mutils::post_object(function, type);
    Message<RecordType>::post_object(function);
}
template<typename RecordType>
std::unique_ptr<SignatureRequest<RecordType>> SignatureRequest<RecordType>::from_bytes(mutils::DeserializationManager* m, uint8_t const* buffer) {
    std::size_t bytes_read = 0;
    MessageType message_type;
    std::memcpy(&message_type, buffer + bytes_read, sizeof(MessageType));
    bytes_read += sizeof(MessageType);

    int sender_id;
    std::memcpy(&sender_id, buffer + bytes_read, sizeof(sender_id));
    bytes_read += sizeof(sender_id);
    std::shared_ptr<body_type> body_shared = mutils::from_bytes<body_type>(m, buffer + bytes_read);
    return std::make_unique<SignatureRequest<RecordType>>(sender_id, body_shared);
}

template<typename RecordType>
std::ostream& operator<<(std::ostream& out, const SignatureRequest<RecordType>& message) {
    return out << "SignatureRequest with body: " << *(std::static_pointer_cast<SignatureResponse<RecordType>::body_type>(message.body));
}

}  // namespace messaging
}  // namespace adq
