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
std::shared_ptr<messaging::ByteBody<RecordType>> CryptoLibrary::rsa_blind(const messaging::ValueTuple<RecordType>& value) {
    std::size_t bytes_size = mutils::bytes_size(value);
    uint8_t value_bytes[bytes_size];
    mutils::to_bytes(value, value_bytes);
    return std::make_shared<messaging::ByteBody<RecordType>>(blind_signature_client.make_blind_message(value_bytes, bytes_size));
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

template <typename RecordType>
std::shared_ptr<messaging::ByteBody<RecordType>> CryptoLibrary::rsa_encrypt(
    const messaging::ValueTuple<RecordType>& value, const int target_meter_id) {
    std::size_t value_bytes_length = mutils::bytes_size(value);
    uint8_t value_bytes[value_bytes_length];
    mutils::to_bytes(value, value_bytes);
    // Create an EnvelopeEncryptor for the destination, using its public key
    openssl::EnvelopeEncryptor encryptor(public_keys_by_id.at(target_meter_id), openssl::CipherAlgorithm::AES256_CBC);
    std::vector<uint8_t> encrypted_message = encryptor.make_encrypted_message(value_bytes, value_bytes_length);
    return std::make_shared<messaging::ByteBody<RecordType>>(std::move(encrypted_message));
}

template <typename RecordType>
void CryptoLibrary::rsa_encrypt(messaging::OverlayMessage<RecordType>& message, const int target_id) {
    message.is_encrypted = true;
    if(message.enclosed_body == nullptr) {
        return;
    }
    // Serialize the body to make it a byte array to encrypt
    std::size_t body_bytes_size = mutils::bytes_size(*message.enclosed_body);
    uint8_t body_bytes[body_bytes_size];
    mutils::to_bytes(*message.enclosed_body, body_bytes);

    // Create an EnvelopeEncryptor for the destination, using its public key
    openssl::EnvelopeEncryptor encryptor(public_keys_by_id.at(target_id), openssl::CipherAlgorithm::AES256_CBC);
    // Encrypted body format: encrypted session key, IV, encrypted payload
    std::vector<uint8_t> encrypted_body = encryptor.make_encrypted_message(body_bytes, body_bytes_size);
    message.enclosed_body = std::make_shared<messaging::ByteBody<RecordType>>(std::move(encrypted_body));
}

template <typename RecordType>
void CryptoLibrary::rsa_decrypt(messaging::OverlayMessage<RecordType>& message) {
    message.is_encrypted = false;
    if(message.enclosed_body == nullptr) {
        return;
    }
    std::shared_ptr<messaging::ByteBody<RecordType>> encrypted_body =
        std::static_pointer_cast<messaging::ByteBody<RecordType>>(message.enclosed_body);
    // Encrypted body format: encrypted session key, IV, encrypted payload
    std::size_t key_iv_size = my_decryptor.get_IV_size() + my_decryptor.get_encrypted_key_size();
    // The plaintext will be no larger than the ciphertext, and possibly smaller
    std::vector<uint8_t> decrypted_body(encrypted_body->size() - key_iv_size);
    my_decryptor.init(encrypted_body->data(), encrypted_body->data() + my_decryptor.get_encrypted_key_size());
    std::size_t bytes_written = my_decryptor.decrypt_bytes(encrypted_body->data() + key_iv_size,
                                                           encrypted_body->size() - key_iv_size,
                                                           decrypted_body.data());
    bytes_written += my_decryptor.finalize(decrypted_body.data() + bytes_written);
    // Shrink the array to fit
    assert(bytes_written <= decrypted_body.size());
    decrypted_body.resize(bytes_written);
    // Deserialize the decrypted payload
    message.enclosed_body = mutils::from_bytes<messaging::MessageBody<RecordType>>(nullptr, decrypted_body.data());
}

template <typename RecordType>
std::shared_ptr<messaging::ByteBody<RecordType>> CryptoLibrary::rsa_sign_blinded(const messaging::ByteBody<RecordType>& blinded_message) {
    return std::make_shared<messaging::ByteBody<RecordType>>(
        my_blind_signer.sign_blinded(blinded_message.data(), blinded_message.size()));
}

}  // namespace adq