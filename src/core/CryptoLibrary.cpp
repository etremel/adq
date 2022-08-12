/*
 * This file contains the non-templated member functions of CryptoLibrary.
 * The templated member functions are in CryptoLibrary_impl.hpp, but non-templated
 * functions can't go there.
 */

#include "adq/core/CryptoLibrary.hpp"

#include "adq/openssl/envelope_encryption.hpp"
#include "adq/openssl/signature.hpp"

#include <map>
#include <memory>
#include <string>

namespace adq {

// A simple helper function that lets me use a for loop in an initializer list
std::map<int, openssl::EnvelopeKey> CryptoLibrary::construct_key_map_from_files(const std::map<int, std::string>& key_files_by_id) {
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

}  // namespace adq