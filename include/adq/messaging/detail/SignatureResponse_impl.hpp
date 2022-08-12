#pragma once

#include "../SignatureResponse.hpp"

#include "adq/messaging/Message.hpp"
#include "adq/messaging/MessageType.hpp"
#include "adq/mutils-serialization/SerializationSupport.hpp"

#include <cstdint>
#include <cstring>
#include <memory>
#include <ostream>

namespace adq {

namespace messaging {

// Re-declare this constant to make the compiler happy
template <typename RecordType>
const constexpr MessageType SignatureResponse<RecordType>::type;

template <typename RecordType>
std::ostream& operator<<(std::ostream& out, const SignatureResponse<RecordType>& message) {
    return out << "SignatureResponse with body: " << *(std::static_pointer_cast<SignatureResponse<RecordType>::body_type>(message.body));
}

template <typename RecordType>
std::size_t SignatureResponse<RecordType>::bytes_size() const {
    return mutils::bytes_size(type) + Message<RecordType>::bytes_size();
}

template <typename RecordType>
std::size_t SignatureResponse<RecordType>::to_bytes(uint8_t* buffer) const {
    std::size_t bytes_written = mutils::to_bytes(type, buffer);
    bytes_written += Message<RecordType>::to_bytes(buffer + bytes_written);
    return bytes_written;
}

template <typename RecordType>
void SignatureResponse<RecordType>::post_object(const std::function<void(const uint8_t* const, std::size_t)>& function) const {
    mutils::post_object(function, type);
    Message<RecordType>::post_object(function);
}

template <typename RecordType>
std::unique_ptr<SignatureResponse<RecordType>> SignatureResponse<RecordType>::from_bytes(mutils::DeserializationManager* m, uint8_t const* buffer) {
    std::size_t bytes_read = 0;
    MessageType message_type;
    std::memcpy(&message_type, buffer + bytes_read, sizeof(MessageType));
    bytes_read += sizeof(MessageType);

    int sender_id;
    std::memcpy(&sender_id, buffer + bytes_read, sizeof(int));
    bytes_read += sizeof(int);
    std::shared_ptr<body_type> body_shared = mutils::from_bytes<body_type>(m, buffer + bytes_read);
    return std::make_unique<SignatureResponse<RecordType>>(sender_id, body_shared);
}

}  // namespace messaging
}  // namespace adq