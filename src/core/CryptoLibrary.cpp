/*
 * This file contains the non-templated member functions of CryptoLibrary.
 * The templated member functions are in CryptoLibrary_impl.hpp, but non-templated
 * functions can't go there.
 */

#include "adq/core/CryptoLibrary.hpp"

#include "adq/messaging/Message.hpp"
#include "adq/messaging/OverlayMessage.hpp"
#include "adq/mutils-serialization/SerializationSupport.hpp"
#include "adq/openssl/envelope_encryption.hpp"
#include "adq/openssl/signature.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace adq {

// A simple helper function that lets me use a for loop in an initializer list
std::map<int, openssl::EnvelopeKey> construct_key_map_from_files(const std::map<int, std::string>& key_files_by_id) {
    std::map<int, openssl::EnvelopeKey> keys_by_id;
    for(const auto& id_filename_pair : key_files_by_id) {
        keys_by_id.emplace(id_filename_pair.first, openssl::EnvelopeKey::from_pem_public(id_filename_pair.second));
    }
    return keys_by_id;
}

CryptoLibrary::CryptoLibrary(const std::string& private_key_filename,
                             const std::map<int, std::string>& public_key_files_by_id)
    : my_private_key(openssl::EnvelopeKey::from_pem_private(private_key_filename)),
      public_keys_by_id(construct_key_map_from_files(public_key_files_by_id)),
      my_signer(my_private_key, openssl::DigestAlgorithm::SHA256),
      my_blind_signer(my_private_key),
      blind_signature_client(public_keys_by_id.at(UTILITY_NODE_ID)),
      my_decryptor(my_private_key, openssl::CipherAlgorithm::AES256_CBC) {}

void CryptoLibrary::rsa_encrypt(messaging::OverlayMessage& message, const int target_id) {
    message.is_encrypted = true;
    if(message.body == nullptr) {
        return;
    }
    // Serialize the body to make it a byte array to encrypt
    std::size_t body_bytes_size = mutils::bytes_size(*message.body);
    uint8_t body_bytes[body_bytes_size];
    mutils::to_bytes(*message.body, body_bytes);

    // Create an EnvelopeEncryptor for the destination, using its public key
    openssl::EnvelopeEncryptor encryptor(public_keys_by_id.at(target_id), openssl::CipherAlgorithm::AES256_CBC);
    // Encrypted body format: encrypted session key, IV, encrypted payload
    std::vector<uint8_t> encrypted_body = encryptor.make_encrypted_message(body_bytes, body_bytes_size);
    message.body = std::make_shared<messaging::ByteBody>(std::move(encrypted_body));
}

void CryptoLibrary::rsa_decrypt(messaging::OverlayMessage& message) {
    message.is_encrypted = false;
    if(message.body == nullptr) {
        return;
    }
    std::shared_ptr<messaging::ByteBody> encrypted_body = std::static_pointer_cast<messaging::ByteBody>(message.body);
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
    message.body = mutils::from_bytes<messaging::MessageBody>(nullptr, decrypted_body.data());
}

std::shared_ptr<messaging::ByteBody> CryptoLibrary::rsa_sign_blinded(const messaging::ByteBody& blinded_message) {
    return std::make_shared<messaging::ByteBody>(
        my_blind_signer.sign_blinded(blinded_message.data(), blinded_message.size()));
}

}  // namespace adq
