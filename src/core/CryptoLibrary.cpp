/*
 * This file contains the non-templated member functions of CryptoLibrary.
 * The templated member functions are in CryptoLibrary_impl.hpp, but non-templated
 * functions can't go there.
 */

#include "adq/core/CryptoLibrary.hpp"

#include "adq/openssl/signature.hpp"

namespace adq {

CryptoLibrary::CryptoLibrary(const std::string& private_key_filename,
                             const std::map<int, std::string>& public_key_files_by_id)
    : my_private_key(openssl::EnvelopeKey::from_pem_private(private_key_filename)),
      my_signer(my_private_key, openssl::DigestAlgorithm::SHA256) {
    for(const auto& id_filename_pair : public_key_files_by_id) {
        public_keys_by_id.emplace(id_filename_pair.first, openssl::EnvelopeKey::from_pem_public(id_filename_pair.second));
    }
}

void CryptoLibrary::rsa_encrypt(messaging::OverlayMessage& message, const int target_id) {
    // TODO
}

void CryptoLibrary::rsa_decrypt(messaging::OverlayMessage& message) {
    // TODO
}

std::shared_ptr<messaging::StringBody> CryptoLibrary::rsa_sign_encrypted(
    const messaging::StringBody& encrypted_message) {
    // TODO
}
void CryptoLibrary::rsa_decrypt_signature(const std::string& blinded_signature,
                                          SignatureArray& signature) {
    // TODO
}
std::shared_ptr<messaging::StringBody> CryptoLibrary::rsa_sign_blinded(const messaging::StringBody& blinded_message) {
    // TODO
}
void CryptoLibrary::rsa_unblind_signature(const std::string& blinded_signature,
                                          SignatureArray& signature) {
    // TODO
}

}  // namespace adq
