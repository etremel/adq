#pragma once

#include "../AggregationMessage.hpp"

#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <memory>
#include <vector>

namespace adq {
namespace messaging {

template<typename RecordType>
bool operator==(const AggregationMessage<RecordType>& lhs, const AggregationMessage<RecordType>& rhs) {
    return lhs.num_contributors == rhs.num_contributors && lhs.query_num == rhs.query_num && (*lhs.body) == (*rhs.body);
}

template <typename RecordType>
bool operator!=(const AggregationMessage<RecordType>& lhs, const AggregationMessage<RecordType>& rhs) {
    return !(lhs == rhs);
}

template <typename RecordType>
std::ostream& operator<<(std::ostream& out, const AggregationMessage<RecordType>& m) {
    return out << *m.get_body() << " | Contributors: " << m.num_contributors;
}

template <typename RecordType>
std::size_t AggregationMessage<RecordType>::bytes_size() const {
    return mutils::bytes_size(type) +
           mutils::bytes_size(num_contributors) +
           mutils::bytes_size(query_num) +
           Message<RecordType>::bytes_size();
}

template <typename RecordType>
std::size_t AggregationMessage<RecordType>::to_bytes(uint8_t* buffer) const {
    std::size_t bytes_written = mutils::to_bytes(type, buffer);
    bytes_written += mutils::to_bytes(num_contributors, buffer + bytes_written);
    bytes_written += mutils::to_bytes(query_num, buffer + bytes_written);
    bytes_written += Message<RecordType>::to_bytes(buffer + bytes_written);
    return bytes_written;
}

template <typename RecordType>
void AggregationMessage<RecordType>::post_object(const std::function<void(const uint8_t* const, std::size_t)>& function) const {
    mutils::post_object(function, type);
    mutils::post_object(function, num_contributors);
    mutils::post_object(function, query_num);
    Message<RecordType>::post_object(function);
}

template <typename RecordType>
std::unique_ptr<AggregationMessage<RecordType>> AggregationMessage<RecordType>::from_bytes(mutils::DeserializationManager* m, const uint8_t* buffer) {
    std::size_t bytes_read = 0;
    MessageType message_type;
    std::memcpy(&message_type, buffer + bytes_read, sizeof(MessageType));
    bytes_read += sizeof(MessageType);

    int num_contributors;
    std::memcpy(&num_contributors, buffer + bytes_read, sizeof(num_contributors));
    bytes_read += sizeof(num_contributors);
    int query_num;
    std::memcpy(&query_num, buffer + bytes_read, sizeof(query_num));
    bytes_read += sizeof(query_num);

    // Superclass deserialization
    int sender_id;
    std::memcpy(&sender_id, buffer + bytes_read, sizeof(sender_id));
    bytes_read += sizeof(sender_id);
    std::unique_ptr<body_type> body = mutils::from_bytes<body_type>(m, buffer + bytes_read);
    auto body_shared = std::shared_ptr<body_type>(std::move(body));
    return std::unique_ptr<AggregationMessage<RecordType>>(new AggregationMessage<RecordType>(sender_id, query_num, body_shared, num_contributors));
}

} /* namespace messaging */
}  // namespace adq
