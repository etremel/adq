#pragma once

#include <adq/util/CryptoLibrary.hpp>
#include <adq/util/Hash.hpp>
#include <cstdint>
#include <cstring>
#include <ostream>
#include <tuple>
#include <vector>

#include "MessageBody.hpp"
#include "MessageBodyType.hpp"
#include "ValueTuple.hpp"

namespace adq {

namespace messaging {

/**
 * Represents a signed (round, value, proxies) tuple that can be contributed to
 * an aggregation query.
 */
struct ValueContribution : public MessageBody {
    static const constexpr MessageBodyType type = MessageBodyType::VALUE_CONTRIBUTION;
    ValueTuple value;
    util::SignatureArray signature;
    ValueContribution(const ValueTuple& value) : value(value) {
        signature.fill(0);
    }
    ValueContribution(const ValueTuple& value, const util::SignatureArray& signature) : value(value), signature(signature) {}
    virtual ~ValueContribution() = default;

    inline bool operator==(const MessageBody& _rhs) const {
        if(auto* rhs = dynamic_cast<const ValueContribution*>(&_rhs))
            return this->value == rhs->value && this->signature == rhs->signature;
        else
            return false;
    }

    //Serialization support
    std::size_t to_bytes(char* buffer) const override;
    void post_object(const std::function<void(char const* const, std::size_t)>& consumer_function) const override;
    std::size_t bytes_size() const override;
    static std::unique_ptr<ValueContribution> from_bytes(mutils::DeserializationManager<>* m, char const* buffer);
};

inline std::ostream& operator<<(std::ostream& stream, const ValueContribution& vc) {
    return stream << "{ValueContribution: " << vc.value << "}";
}

}  // namespace messaging

}  // namespace adq

namespace std {

template <>
struct hash<adq::messaging::ValueContribution> {
    size_t operator()(const adq::messaging::ValueContribution& input) const {
        size_t result = 1;
        adq::util::hash_combine(result, input.signature);
        adq::util::hash_combine(result, input.value);
        return result;
    }
};

}  // namespace std
