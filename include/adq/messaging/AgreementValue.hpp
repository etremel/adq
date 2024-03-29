#pragma once

#include "MessageBody.hpp"
#include "MessageBodyType.hpp"
#include "SignedValue.hpp"

#include <array>
#include <cstdint>
#include <ostream>

namespace adq {

using SignatureArray = std::array<uint8_t, 256>;

namespace messaging {

template <typename RecordType>
struct AgreementValue : public MessageBody<RecordType> {
    static const constexpr MessageBodyType type = MessageBodyType::AGREEMENT_VALUE;
    SignedValue<RecordType> signed_value;
    /** The ID of the node that signed the SignedValue (after accepting it). */
    int accepter_id;
    /** The signature over the entire SignedValue of a node that accepted the value. */
    SignatureArray accepter_signature;

    AgreementValue(const SignedValue<RecordType>& signed_value, const int accepter_id)
        : signed_value(signed_value), accepter_id(accepter_id) {}
    // Member-by-member constructor used only by serialization
    AgreementValue(const SignedValue<RecordType>& signed_value, const int accepter_id, const SignatureArray& signature)
        : signed_value(signed_value), accepter_id(accepter_id), accepter_signature(signature) {}
    virtual ~AgreementValue() = default;
    bool operator==(const MessageBody<RecordType>& _rhs) const override;

    std::size_t bytes_size() const;
    std::size_t to_bytes(uint8_t* buffer) const;
    void post_object(const std::function<void(uint8_t const* const, std::size_t)>&) const;
    static std::unique_ptr<AgreementValue<RecordType>> from_bytes(mutils::DeserializationManager* p, const uint8_t* buffer);
};

template <typename RecordType>
std::ostream& operator<<(std::ostream& out, const AgreementValue<RecordType> val) {
    return out << "{AgreementValue: " << val.signed_value << " accepted by " << val.accepter_id << "}";
}

}  // namespace messaging

}  // namespace adq

#include "detail/AgreementValue_impl.hpp"