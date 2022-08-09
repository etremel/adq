#pragma once

#include "../CryptoLibrary.hpp"

#include "adq/messaging/ByteBody.hpp"
#include "adq/messaging/Message.hpp"
#include "adq/messaging/OverlayMessage.hpp"
#include "adq/messaging/SignedValue.hpp"
#include "adq/messaging/ValueContribution.hpp"
#include "adq/messaging/ValueTuple.hpp"
#include "adq/openssl/blind_signature.hpp"
#include "adq/openssl/envelope_encryption.hpp"
#include "adq/openssl/hash.hpp"
#include "adq/openssl/signature.hpp"

namespace adq {

template <typename RecordType>
std::shared_ptr<messaging::ByteBody> CryptoLibrary::rsa_encrypt(
    const messaging::ValueTuple<RecordType>& value, const int target_meter_id) {
    std::size_t value_bytes_length = mutils::bytes_size(value);
    uint8_t value_bytes[value_bytes_length];
    mutils::to_bytes(value, value_bytes);
    // Create an EnvelopeEncryptor for the destination, using its public key
    openssl::EnvelopeEncryptor encryptor(public_keys_by_id.at(target_meter_id), openssl::CipherAlgorithm::AES256_CBC);
    std::vector<uint8_t> encrypted_message = encryptor.make_encrypted_message(value_bytes, value_bytes_length);
    return std::make_shared<messaging::ByteBody>(std::move(encrypted_message));
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
bool CryptoLibrary::rsa_verify(const messaging::ValueTuple<RecordType>& value,
                               const SignatureArray& signature) {
    openssl::Verifier verifier(public_keys_by_id.at(UTILITY_NODE_ID), openssl::DigestAlgorithm::SHA256);
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

template <typename RecordType>
std::shared_ptr<messaging::ByteBody> CryptoLibrary::rsa_blind(const messaging::ValueTuple<RecordType>& value) {
    std::size_t bytes_size = mutils::bytes_size(value);
    uint8_t value_bytes[bytes_size];
    mutils::to_bytes(value, value_bytes);
    return std::make_shared<messaging::ByteBody>(blind_signature_client.make_blind_message(value_bytes, bytes_size));
}
template <typename RecordType>
void CryptoLibrary::rsa_unblind_signature(const messaging::ValueTuple<RecordType>& value,
                                          const std::vector<uint8_t>& blinded_signature,
                                          SignatureArray& signature) {
    std::size_t bytes_size = mutils::bytes_size(value);
    uint8_t value_bytes[bytes_size];
    mutils::to_bytes(value, value_bytes);
    // Unnecessary extra copy. I should make an unblind_signature that receives the signature array.
    std::vector<uint8_t> temp_signature = blind_signature_client.unblind_signature(
        blinded_signature.data(), blinded_signature.size(), value_bytes, bytes_size);
    std::copy_n(temp_signature.begin(), signature.size(), signature.begin());
}
}  // namespace adq