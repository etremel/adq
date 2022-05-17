/**
 * @file QueryRequest.cpp
 *
 * @date Oct 14, 2016
 * @author edward
 */

#include "adq/messaging/QueryRequest.hpp"
#include "adq/mutils-serialization/SerializationSupport.hpp"

#include <memory>
#include <ostream>

namespace adq {
namespace messaging {

const constexpr MessageType QueryRequest::type;

std::size_t QueryRequest::bytes_size() const {
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

std::size_t QueryRequest::to_bytes(uint8_t* buffer) const {
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

void QueryRequest::post_object(const std::function<void(const uint8_t* const, std::size_t)>& function) const {
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

std::unique_ptr<QueryRequest> QueryRequest::from_bytes(mutils::DeserializationManager* m, const uint8_t* buffer) {
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
    return std::make_unique<QueryRequest>(query_number, select_function_opcode, filter_function_opcode,
                                          aggregate_function_opcode, *select_serialized_args,
                                          *filter_serialized_args, *aggregate_serialized_args);
}

std::ostream& operator<<(std::ostream& out, const QueryRequest& qr) {
    return out << "{QueryRequest: Type=" << qr.request_type << " | query_number=" << qr.query_number << " | time_window=" << qr.time_window << " }";
}

} /* namespace messaging */
} /* namespace adq */
