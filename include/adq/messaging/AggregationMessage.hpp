#pragma once

#include "Message.hpp"
#include "MessageBody.hpp"
#include "MessageBodyType.hpp"
#include "MessageType.hpp"
#include "adq/core/InternalTypes.hpp"
#include "adq/mutils-serialization/SerializationSupport.hpp"
#include "adq/util/Hash.hpp"

#include <cstddef>
#include <iterator>
#include <memory>
#include <ostream>
#include <vector>

namespace adq {
namespace messaging {

/**
 * Wraps a value of type RecordType in a MessageBody so it can be the body of an
 * AggregationMessage.
 *
 * @tparam RecordType The type of data being collected by queries
 */
template <typename RecordType>
struct AggregationMessageValue : public MessageBody {
    RecordType value;

    static const constexpr MessageBodyType type = MessageBodyType::AGGREGATION_VALUE;
    AggregationMessageValue() = default;
    AggregationMessageValue(const RecordType& value) : value(value) {}
    AggregationMessageValue(const AggregationMessageValue&) = default;
    AggregationMessageValue(AggregationMessageValue&&) = default;
    virtual ~AggregationMessageValue() = default;

    AggregationMessageValue<RecordType>& operator=(const AggregationMessageValue<RecordType>& other) {
        value = other.value;
        return *this;
    }
    inline bool operator==(const MessageBody& _rhs) const {
        if(auto* rhs = dynamic_cast<const AggregationMessageValue<RecordType>*>(&_rhs))
            return this->value == rhs->value;
        else
            return false;
    }
    // Serialization support; very simple since there is only one field
    std::size_t bytes_size() const {
        return mutils::bytes_size(type) + mutils::bytes_size(value);
    }
    std::size_t to_bytes(uint8_t* buffer) const {
        std::size_t bytes_written = mutils::to_bytes(type, buffer);
        return bytes_written + mutils::to_bytes(value, buffer + bytes_written);
    }
    void post_object(const std::function<void(uint8_t const* const, std::size_t)>& f) const {
        mutils::post_object(f, type);
        mutils::post_object(f, value);
    }
    static std::unique_ptr<AggregationMessageValue<RecordType>> from_bytes(mutils::DeserializationManager* m, uint8_t const* buffer) {
        /*"Skip past the MessageBodyType, then take the deserialized value
         * and wrap it in a new AggregationMessageValue"*/
        return std::make_unique<AggregationMessageValue<RecordType>>(
            *mutils::from_bytes<RecordType>(m, buffer + sizeof(type)));
    }
};

template <typename RecordType>
std::ostream& operator<<(std::ostream& out, const AggregationMessageValue<RecordType>& v);

/**
 * The messages sent in the Aggregate phase of all versions of the protocol.
 * They carry the query result (or its intermediate value) and the count of
 * data points that contributed to the result.
 */
template <typename RecordType>
class AggregationMessage : public Message {
public:
    static const constexpr MessageType type = MessageType::AGGREGATION;
    using body_type = AggregationMessageValue<RecordType>;
    int num_contributors;
    int query_num;
    AggregationMessage() : Message(0, nullptr), num_contributors(0), query_num(0) {}
    AggregationMessage(int sender_id, int query_num, std::shared_ptr<AggregationMessageValue<RecordType>> value, int num_contributors)
        : Message(sender_id, value), num_contributors(num_contributors), query_num(query_num) {}
    virtual ~AggregationMessage() = default;
    std::shared_ptr<body_type> get_body() { return std::static_pointer_cast<body_type>(body); };
    const std::shared_ptr<body_type> get_body() const { return std::static_pointer_cast<body_type>(body); };

    std::size_t bytes_size() const;
    std::size_t to_bytes(uint8_t* buffer) const;
    void post_object(const std::function<void(uint8_t const* const, std::size_t)>&) const;
    static std::unique_ptr<AggregationMessage> from_bytes(mutils::DeserializationManager* p, const uint8_t* buffer);

    friend struct std::hash<AggregationMessage>;
};

template <typename RecordType>
bool operator==(const AggregationMessage<RecordType>& lhs, const AggregationMessage<RecordType>& rhs);

template <typename RecordType>
bool operator!=(const AggregationMessage<RecordType>& lhs, const AggregationMessage<RecordType>& rhs);

template <typename RecordType>
std::ostream& operator<<(std::ostream& out, const AggregationMessage<RecordType>& m);

} /* namespace messaging */
} /* namespace adq */

namespace std {

template <typename RecordType>
struct hash<adq::messaging::AggregationMessageValue<RecordType>> {
    size_t operator()(const adq::messaging::AggregationMessageValue<RecordType>& input) const {
        // Just hash the single member of the struct
        std::hash<RecordType> hasher;
        return hasher(input.value);
    }
};

template <typename RecordType>
struct hash<adq::messaging::AggregationMessage<RecordType>> {
    size_t operator()(const adq::messaging::AggregationMessage<RecordType>& input) const {
        using adq::util::hash_combine;

        size_t result = 1;
        hash_combine(result, input.num_contributors);
        hash_combine(result, input.query_num);
        hash_combine(result, *std::static_pointer_cast<adq::messaging::AggregationMessageValue<RecordType>>(input.body));
        return result;
    }
};

}  // namespace std

#include "detail/AggregationMessage_impl.hpp"
