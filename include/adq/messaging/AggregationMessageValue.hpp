#pragma once

#include "adq/messaging/MessageBody.hpp"
#include "adq/messaging/MessageBodyType.hpp"
#include "adq/mutils-serialization/SerializationSupport.hpp"

#include <cstddef>
#include <functional>
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
struct AggregationMessageValue : public MessageBody<RecordType> {
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
    bool operator==(const MessageBody<RecordType>& _rhs) const override {
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
std::ostream& operator<<(std::ostream& out, const AggregationMessageValue<RecordType>& v) {
    out << v.value;
    return out;
}

}  // namespace messaging
}  // namespace adq

namespace std {

template <typename RecordType>
struct hash<adq::messaging::AggregationMessageValue<RecordType>> {
    size_t operator()(const adq::messaging::AggregationMessageValue<RecordType>& input) const {
        // Just hash the single member of the struct
        std::hash<RecordType> hasher;
        return hasher(input.value);
    }
};

}  // namespace std