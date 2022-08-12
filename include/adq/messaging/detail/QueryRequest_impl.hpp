#pragma once

#include "../QueryRequest.hpp"

#include "adq/mutils-serialization/SerializationSupport.hpp"

#include <memory>
#include <ostream>

namespace adq {
namespace messaging {

template <typename RecordType>
const constexpr MessageType QueryRequest<RecordType>::type;

template <typename RecordType>
bool QueryRequest<RecordType>::operator==(const Message<RecordType>& _rhs) const {
    if(auto* rhs = dynamic_cast<const QueryRequest<RecordType>*>(&_rhs))
        return this->select_function_opcode == rhs->select_function_opcode &&
               this->filter_function_opcode == rhs->filter_function_opcode &&
               this->aggregate_function_opcode == rhs->aggregate_function_opcode &&
               this->query_number == rhs->query_number;
    else
        return false;
}

template <typename RecordType>
std::size_t QueryRequest<RecordType>::bytes_size() const {
    return mutils::bytes_size(type) +
           mutils::bytes_size(sender_id) +
           mutils::bytes_size(query_number) +
           mutils::bytes_size(select_function_opcode) +
           mutils::bytes_size(filter_function_opcode) +
           mutils::bytes_size(aggregate_function_opcode) +
           mutils::bytes_size(select_serialized_args) +
           mutils::bytes_size(filter_serialized_args) +
           mutils::bytes_size(aggregate_serialized_args);
}

template <typename RecordType>
std::size_t QueryRequest<RecordType>::to_bytes(uint8_t* buffer) const {
    std::size_t bytes_written = mutils::to_bytes(type, buffer);
    bytes_written += mutils::to_bytes(sender_id, buffer + bytes_written);
    bytes_written += mutils::to_bytes(query_number, buffer + bytes_written);
    bytes_written += mutils::to_bytes(select_function_opcode, buffer + bytes_written);
    bytes_written += mutils::to_bytes(filter_function_opcode, buffer + bytes_written);
    bytes_written += mutils::to_bytes(aggregate_function_opcode, buffer + bytes_written);
    bytes_written += mutils::to_bytes(select_serialized_args, buffer + bytes_written);
    bytes_written += mutils::to_bytes(filter_serialized_args, buffer + bytes_written);
    bytes_written += mutils::to_bytes(aggregate_serialized_args, buffer + bytes_written);
    return bytes_written;
}

template <typename RecordType>
void QueryRequest<RecordType>::post_object(const std::function<void(const uint8_t* const, std::size_t)>& function) const {
    mutils::post_object(function, type);
    mutils::post_object(function, sender_id);
    mutils::post_object(function, query_number);
    mutils::post_object(function, select_function_opcode);
    mutils::post_object(function, filter_function_opcode);
    mutils::post_object(function, aggregate_function_opcode);
    mutils::post_object(function, select_serialized_args);
    mutils::post_object(function, filter_serialized_args);
    mutils::post_object(function, aggregate_serialized_args);
}

template <typename RecordType>
std::unique_ptr<QueryRequest<RecordType>> QueryRequest<RecordType>::from_bytes(mutils::DeserializationManager* m, const uint8_t* buffer) {
    std::size_t bytes_read = 0;
    MessageType message_type;
    std::memcpy(&message_type, buffer + bytes_read, sizeof(MessageType));
    bytes_read += sizeof(MessageType);

    int sender_id;
    std::memcpy(&sender_id, buffer + bytes_read, sizeof(sender_id));
    bytes_read += sizeof(sender_id);

    int query_number;
    std::memcpy(&query_number, buffer + bytes_read, sizeof(query_number));
    bytes_read += sizeof(query_number);

    Opcode select_function_opcode;
    std::memcpy(&select_function_opcode, buffer + bytes_read, sizeof(select_function_opcode));
    bytes_read += sizeof(select_function_opcode);

    Opcode filter_function_opcode;
    std::memcpy(&filter_function_opcode, buffer + bytes_read, sizeof(filter_function_opcode));
    bytes_read += sizeof(filter_function_opcode);

    Opcode aggregate_function_opcode;
    std::memcpy(&aggregate_function_opcode, buffer + bytes_read, sizeof(aggregate_function_opcode));
    bytes_read += sizeof(aggregate_function_opcode);

    std::unique_ptr<std::vector<uint8_t>> select_serialized_args =
        mutils::from_bytes<std::vector<uint8_t>>(nullptr, buffer + bytes_read);
    bytes_read += mutils::bytes_size(*select_serialized_args);

    std::unique_ptr<std::vector<uint8_t>> filter_serialized_args =
        mutils::from_bytes<std::vector<uint8_t>>(nullptr, buffer + bytes_read);
    bytes_read += mutils::bytes_size(*filter_serialized_args);

    std::unique_ptr<std::vector<uint8_t>> aggregate_serialized_args =
        mutils::from_bytes<std::vector<uint8_t>>(nullptr, buffer + bytes_read);
    bytes_read += mutils::bytes_size(*aggregate_serialized_args);

    // Unnecessary extra copy of the byte arrays
    return std::make_unique<QueryRequest<RecordType>>(query_number, select_function_opcode, filter_function_opcode,
                                                      aggregate_function_opcode, *select_serialized_args,
                                                      *filter_serialized_args, *aggregate_serialized_args);
}

template <typename RecordType>
std::ostream& operator<<(std::ostream& out, const QueryRequest<RecordType>& qr) {
    return out << "{QueryRequest: query_number=" << qr.query_number
               << " | select_opcode=" << qr.select_function_opcode
               << " | filter_opcode=" << qr.filter_function_opcode
               << " | aggregate_opcode=" << qr.aggregate_function_opcode << "}";
}

} /* namespace messaging */
} /* namespace adq */
