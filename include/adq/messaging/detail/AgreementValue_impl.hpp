#include "../AgreementValue.hpp"

#include "adq/mutils-serialization/SerializationSupport.hpp"

#include <cstring>
#include <memory>

namespace adq {
namespace messaging {

template <typename RecordType>
const constexpr MessageBodyType AgreementValue<RecordType>::type;

template <typename RecordType>
bool AgreementValue<RecordType>::operator==(const MessageBody<RecordType>& _rhs) const {
    if(auto* rhs = dynamic_cast<const AgreementValue<RecordType>*>(&_rhs))
        return this->signed_value == rhs->signed_value &&
               this->accepter_id == rhs->accepter_id &&
               this->accepter_signature == rhs->accepter_signature;
    else
        return false;
}

template <typename RecordType>
std::size_t AgreementValue<RecordType>::bytes_size() const {
    // Don't add sizeof(MessageBodyType) because SignedValue already adds it
    return mutils::bytes_size(signed_value) +
           mutils::bytes_size(accepter_id) +
           (accepter_signature.size() * sizeof(SignatureArray::value_type));
}

template <typename RecordType>
std::size_t AgreementValue<RecordType>::to_bytes(uint8_t* buffer) const {
    std::size_t bytes_written = 0;
    bytes_written += mutils::to_bytes(signed_value, buffer);
    // Rewrite the first two bytes of the buffer to change the MessageBodyType
    mutils::to_bytes(type, buffer);
    // Now add the AgreementValue fields
    bytes_written += mutils::to_bytes(accepter_id, buffer + bytes_written);
    std::memcpy(buffer + bytes_written, accepter_signature.data(),
                accepter_signature.size() * sizeof(SignatureArray::value_type));
    bytes_written += accepter_signature.size() * sizeof(SignatureArray::value_type);
    return bytes_written;
}

template <typename RecordType>
void AgreementValue<RecordType>::post_object(const std::function<void(uint8_t const* const, std::size_t)>& consumer) const {
    // This avoids needing to rewrite MessageBodyType after calling post_object(signed_value)
    uint8_t buffer[bytes_size()];
    to_bytes(buffer);
    consumer(buffer, bytes_size());
}

template <typename RecordType>
std::unique_ptr<AgreementValue<RecordType>> AgreementValue<RecordType>::from_bytes(mutils::DeserializationManager* p, const uint8_t* buffer) {
    std::unique_ptr<SignedValue<RecordType>> signedval = mutils::from_bytes<SignedValue<RecordType>>(p, buffer);
    std::size_t bytes_read = mutils::bytes_size(*signedval);
    int accepter_id;
    std::memcpy((uint8_t*)&accepter_id, buffer + bytes_read, sizeof(accepter_id));
    bytes_read += sizeof(accepter_id);

    SignatureArray signature;
    std::memcpy(signature.data(), buffer + bytes_read, signature.size() * sizeof(SignatureArray::value_type));
    bytes_read += signature.size() * sizeof(SignatureArray::value_type);

    // Ugh, unnecessary copy of the SignedValue. Maybe I should store it by unique_ptr instead of by value.
    return std::make_unique<AgreementValue<RecordType>>(*signedval, accepter_id, signature);
}

}  // namespace messaging
}  // namespace adq
