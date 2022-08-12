#pragma once

#include "AggregationMessageValue.hpp"
#include "Message.hpp"
#include "MessageBody.hpp"
#include "MessageBodyType.hpp"
#include "MessageType.hpp"
#include "adq/core/InternalTypes.hpp"
#include "adq/mutils-serialization/SerializationSupport.hpp"
#include "adq/util/Hash.hpp"

#include <cstddef>
#include <memory>
#include <ostream>
#include <vector>

namespace adq {
namespace messaging {

/**
 * The messages sent in the Aggregate phase of all versions of the protocol.
 * They carry the query result (or its intermediate value) and the count of
 * data points that contributed to the result.
 */
template <typename RecordType>
class AggregationMessage : public Message<RecordType> {
public:
    static const constexpr MessageType type = MessageType::AGGREGATION;
    using body_type = AggregationMessageValue<RecordType>;
    // Tell C++ I want to use inheritance with this base class member
    using Message<RecordType>::body;
    int num_contributors;
    int query_num;
    AggregationMessage() : Message<RecordType>(0, nullptr), num_contributors(0), query_num(0) {}
    AggregationMessage(int sender_id, int query_num, std::shared_ptr<AggregationMessageValue<RecordType>> value, int num_contributors)
        : Message<RecordType>(sender_id, value),
          num_contributors(num_contributors),
          query_num(query_num) {}
    virtual ~AggregationMessage() = default;
    /** Returns a pointer to the message body cast to the correct type for this message */
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
