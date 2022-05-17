#pragma once

#include <iostream>
#include <map>
#include <memory>
#include <utility>

#include "MessageBody.hpp"
#include "MessageBodyType.hpp"
#include "ValueContribution.hpp"

namespace adq {

using SignatureArray = std::array<uint8_t, 256>;

namespace messaging {

template <typename RecordType>
class SignedValue : public MessageBody {
public:
    static const constexpr MessageBodyType type = MessageBodyType::SIGNED_VALUE;
    std::shared_ptr<ValueContribution<RecordType>> value;
    /** Maps the meter ID of a meter to that meter's signature on this
     * message's ValueContribution. */
    std::map<int, SignatureArray> signatures;
    SignedValue() : value(nullptr) {}
    SignedValue(const std::shared_ptr<ValueContribution<RecordType>>& value,
                const std::map<int, SignatureArray>& signatures) : value(value), signatures(signatures) {}

    inline bool operator==(const MessageBody& _rhs) const {
        if(auto* rhs = dynamic_cast<const SignedValue*>(&_rhs))
            return (rhs->value == nullptr ? value == rhs->value : *value == *(rhs->value)) && this->signatures == rhs->signatures;
        else
            return false;
    }
    std::size_t bytes_size() const override;
    std::size_t to_bytes(uint8_t* buffer) const override;
    void post_object(const std::function<void(uint8_t const* const, std::size_t)>& function) const override;
    static std::unique_ptr<SignedValue> from_bytes(mutils::DeserializationManager* p, const uint8_t* buffer);

private:
    // Helper functions for the signatures map
    // I don't have time to make this generic for all std::maps
    static std::size_t bytes_size(const std::map<int, SignatureArray>& sig_map);
    static std::size_t to_bytes(const std::map<int, SignatureArray>& sig_map, uint8_t* buffer);
    static std::unique_ptr<std::map<int, SignatureArray>> from_bytes_map(mutils::DeserializationManager* p, const uint8_t* buffer);
};

template <typename RecordType>
std::ostream& operator<<(std::ostream& out, const SignedValue<RecordType>& s) {
    out << "{SignedValue: " << *s.value << "| Signatures from: " << '[';
    for(const auto& id_sig : s.signatures) {
        out << id_sig.first << ", ";
    }
    out << "\b\b]"
        << "}";
    return out;
}

}  // namespace messaging
}  // namespace adq

#include "detail/SignedValue_impl.hpp"
