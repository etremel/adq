#pragma once

#include "Message.hpp"
#include "MessageType.hpp"
#include "adq/core/QueryFunctions.hpp"
#include "adq/mutils-serialization/SerializationSupport.hpp"

#include <functional>
#include <memory>
#include <iostream>

namespace adq {

namespace messaging {

class QueryRequest : public Message {
public:
    static const constexpr MessageType type = MessageType::QUERY_REQUEST;
    const int query_number;
    const Opcode select_function_opcode;
    const Opcode filter_function_opcode;
    const Opcode aggregate_function_opcode;
    const std::vector<uint8_t> select_serialized_args;
    const std::vector<uint8_t> filter_serialized_args;
    const std::vector<uint8_t> aggregate_serialized_args;

    QueryRequest(const int query_number, const Opcode select_function, const Opcode filter_function,
                 const Opcode aggregate_function, const std::vector<uint8_t>& select_serialized_args,
                 const std::vector<uint8_t>& filter_serialized_args, const std::vector<uint8_t>& aggregate_serialized_args)
        : Message(UTILITY_NODE_ID, nullptr),  // hack, these fields should really be the body of the message. Would it hurt to make a QueryRequestMessageBody?
          query_number(query_number),
          select_function_opcode(select_function),
          filter_function_opcode(filter_function),
          aggregate_function_opcode(aggregate_function),
          select_serialized_args(select_serialized_args),
          filter_serialized_args(filter_serialized_args),
          aggregate_serialized_args(aggregate_serialized_args) {}
    virtual ~QueryRequest() = default;
    inline bool operator==(const Message& _rhs) const {
        if(auto* rhs = dynamic_cast<const QueryRequest*>(&_rhs))
            return this->select_function_opcode == rhs->select_function_opcode &&
                   this->filter_function_opcode == rhs->filter_function_opcode &&
                   this->aggregate_function_opcode == rhs->aggregate_function_opcode &&
                   this->query_number == rhs->query_number;
        else
            return false;
    }
    std::size_t bytes_size() const;
    std::size_t to_bytes(uint8_t* buffer) const;
    void post_object(const std::function<void(uint8_t const* const, std::size_t)>& consumer) const;
    static std::unique_ptr<QueryRequest> from_bytes(mutils::DeserializationManager* m, const uint8_t* buffer);
};

std::ostream& operator<<(std::ostream& out, const QueryRequest& qr);

struct QueryNumLess {
    bool operator()(const QueryRequest& lhs, const QueryRequest& rhs) const {
        return lhs.query_number < rhs.query_number;
    }
};

struct QueryNumGreater {
    bool operator()(const QueryRequest& lhs, const QueryRequest& rhs) const {
        return lhs.query_number > rhs.query_number;
    }
};

} /* namespace messaging */
}  // namespace adq
