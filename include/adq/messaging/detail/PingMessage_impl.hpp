#pragma once

#include "../PingMessage.hpp"

#include <cassert>
#include <cstddef>
#include <cstring>
#include <iostream>

namespace adq {
namespace messaging {

template <typename RecordType>
const constexpr MessageType PingMessage<RecordType>::type;

template <typename RecordType>
std::size_t PingMessage<RecordType>::bytes_size() const {
    return mutils::bytes_size(type) +
           mutils::bytes_size(sender_id) +
           mutils::bytes_size(is_response);
}

// This completely overrides Message::to_bytes, since PingMessage ignores the body field
template <typename RecordType>
std::size_t PingMessage<RecordType>::to_bytes(uint8_t* buffer) const {
    std::size_t bytes_written = 0;
    std::memcpy(buffer + bytes_written, &type, sizeof(MessageType));
    bytes_written += sizeof(MessageType);
    std::memcpy(buffer + bytes_written, &sender_id, sizeof(sender_id));
    bytes_written += sizeof(sender_id);
    std::memcpy(buffer + bytes_written, &is_response, sizeof(is_response));
    bytes_written += sizeof(is_response);
    return bytes_written;
}

template <typename RecordType>
void PingMessage<RecordType>::post_object(const std::function<void(const uint8_t*, std::size_t)>& consumer) const {
    mutils::post_object(consumer, type);
    mutils::post_object(consumer, sender_id);
    mutils::post_object(consumer, is_response);
}

template <typename RecordType>
std::unique_ptr<PingMessage<RecordType>> PingMessage<RecordType>::from_bytes(mutils::DeserializationManager* m, const uint8_t* buffer) {
    std::size_t bytes_read = 0;
    MessageType message_type;
    std::memcpy(&message_type, buffer + bytes_read, sizeof(MessageType));
    bytes_read += sizeof(MessageType);
    assert(message_type == MessageType::PING);
    int sender_id;
    std::memcpy(&sender_id, buffer + bytes_read, sizeof(int));
    bytes_read += sizeof(int);
    bool is_response;
    std::memcpy(&is_response, buffer + bytes_read, sizeof(bool));
    bytes_read += sizeof(bool);
    return std::make_unique<PingMessage<RecordType>>(sender_id, is_response);
}

template <typename RecordType>
std::ostream& operator<<(std::ostream& out, const PingMessage<RecordType>& message) {
    return out << "Ping from " << message.sender_id << ", is_response = " << std::boolalpha << message.is_response;
}

}  // namespace messaging
}  // namespace adq
