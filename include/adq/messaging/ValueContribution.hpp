#pragma once

#include "MessageBody.hpp"
#include "MessageBodyType.hpp"
#include "ValueTuple.hpp"
#include "adq/util/Hash.hpp"

#include <cstdint>
#include <cstring>
#include <ostream>
#include <tuple>
#include <vector>

namespace adq {

using SignatureArray = std::array<uint8_t, 256>;

namespace messaging {

/**
 * Represents a signed (round, value, proxies) tuple that can be contributed to
 * an aggregation query.
 */
template <typename RecordType>
struct ValueContribution : public MessageBody<RecordType> {
    static const constexpr MessageBodyType type = MessageBodyType::VALUE_CONTRIBUTION;
    ValueTuple<RecordType> value_tuple;
    SignatureArray signature;
    ValueContribution(const ValueTuple<RecordType>& value) : value_tuple(value) {
        signature.fill(0);
    }
    ValueContribution(const ValueTuple<RecordType>& value, const SignatureArray& signature) : value_tuple(value), signature(signature) {}
    virtual ~ValueContribution() = default;

    bool operator==(const MessageBody<RecordType>& _rhs) const override;

    // Serialization support
    std::size_t to_bytes(uint8_t* buffer) const override;
    void post_object(const std::function<void(uint8_t const* const, std::size_t)>& consumer_function) const override;
    std::size_t bytes_size() const override;
    static std::unique_ptr<ValueContribution<RecordType>> from_bytes(mutils::DeserializationManager* m, uint8_t const* buffer);
};

template <typename RecordType>
std::ostream& operator<<(std::ostream& stream, const ValueContribution<RecordType>& vc) {
    return stream << "{ValueContribution: " << vc.value_tuple << "}";
}

}  // namespace messaging

}  // namespace adq

namespace std {

template <typename RecordType>
struct hash<adq::messaging::ValueContribution<RecordType>> {
    size_t operator()(const adq::messaging::ValueContribution<RecordType>& input) const {
        size_t result = 1;
        adq::util::hash_combine(result, input.signature);
        adq::util::hash_combine(result, input.value_tuple);
        return result;
    }
};

}  // namespace std

#include "detail/ValueContribution_impl.hpp"