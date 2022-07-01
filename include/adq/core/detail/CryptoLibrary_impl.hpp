#pragma once

#include "../CryptoLibrary.hpp"

#include "adq/messaging/OverlayMessage.hpp"
#include "adq/messaging/SignedValue.hpp"
#include "adq/messaging/StringBody.hpp"
#include "adq/messaging/ValueContribution.hpp"
#include "adq/messaging/ValueTuple.hpp"
#include "adq/openssl/hash.hpp"
#include "adq/openssl/signature.hpp"

namespace adq {

template <typename RecordType>
std::shared_ptr<messaging::StringBody> CryptoLibrary::rsa_encrypt(
    const messaging::ValueTuple<RecordType>& value, const int target_meter_id) {
    //TODO
}

template <typename RecordType>
void CryptoLibrary::rsa_sign(const messaging::ValueContribution<RecordType>& value, SignatureArray& signature) {
    my_signer.init();
    std::size_t value_bytes_length = mutils::bytes_size(value);
    uint8_t value_bytes[value_bytes_length];
    mutils::to_bytes(value, value_bytes);
    my_signer.add_bytes(value_bytes, value_bytes_length);
    my_signer.finalize(signature.data());
}

template <typename RecordType>
void CryptoLibrary::rsa_sign(const messaging::SignedValue<RecordType>& value, SignatureArray& signature) {
    my_signer.init();
    std::size_t value_bytes_length = mutils::bytes_size(value);
    uint8_t value_bytes[value_bytes_length];
    mutils::to_bytes(value, value_bytes);
    my_signer.add_bytes(value_bytes, value_bytes_length);
    my_signer.finalize(signature.data());
}

template <typename RecordType>
bool CryptoLibrary::rsa_verify(const messaging::ValueContribution<RecordType>& value,
                               const SignatureArray& signature, const int signer_id) {
    openssl::Verifier verifier(public_keys_by_id.at(signer_id), openssl::DigestAlgorithm::SHA256);
    verifier.init();
    std::size_t value_bytes_length = mutils::bytes_size(value);
    uint8_t value_bytes[value_bytes_length];
    mutils::to_bytes(value, value_bytes);
    verifier.add_bytes(value_bytes, value_bytes_length);
    return verifier.finalize(signature.data(), signature.size());
}

template <typename RecordType>
bool CryptoLibrary::rsa_verify(const messaging::SignedValue<RecordType>& value,
                               const SignatureArray& signature, const int signer_id) {
    openssl::Verifier verifier(public_keys_by_id.at(signer_id), openssl::DigestAlgorithm::SHA256);
    verifier.init();
    std::size_t value_bytes_length = mutils::bytes_size(value);
    uint8_t value_bytes[value_bytes_length];
    mutils::to_bytes(value, value_bytes);
    verifier.add_bytes(value_bytes, value_bytes_length);
    return verifier.finalize(signature.data(), signature.size());
}

template<typename RecordType>
std::shared_ptr<messaging::StringBody> CryptoLibrary::rsa_blind(const messaging::ValueTuple<RecordType>& value) {
    //TODO
}

}  // namespace adq