#include <adq/mutils-serialization/SerializationSupport.hpp>
#include <adq/openssl/blind_signature.hpp>
#include <adq/openssl/envelope_encryption.hpp>
#include <adq/openssl/signature.hpp>
#include <adq/openssl/openssl_exception.hpp>

#include <cassert>
#include <cstdint>
#include <iomanip>
#include <iostream>

class StringObject : public mutils::ByteRepresentable {
    uint32_t id;
    std::string message;

public:
    void append(const std::string& words) {
        message += words;
    }
    void clear() {
        message.clear();
    }
    std::string print() const {
        return std::to_string(id) + ": " + message;
    }

    StringObject(int id, const std::string& s = "") : id(id), message(s) {}

    DEFAULT_SERIALIZATION_SUPPORT(StringObject, id, message);
};

void print_byte_array(const uint8_t* byte_buffer, std::size_t size) {
    std::cout << std::hex;
    for(std::size_t i = 0; i < size; ++i) {
        std::cout << std::setw(2) << std::setfill('0') << (int)byte_buffer[i];
    }
    std::cout << std::dec << std::endl;
}

void test_envelope_encryption(openssl::EnvelopeKey private_key, openssl::EnvelopeKey public_key) {
    openssl::EnvelopeEncryptor encryptor(public_key, openssl::CipherAlgorithm::AES256_CBC);
    StringObject test_object(123, "Secret message to encrypt...secret message to encrypt...secret message to encrypt...secret message to encrypt");
    std::size_t buffer_size = mutils::bytes_size(test_object);
    uint8_t bytes_to_encrypt[buffer_size];
    mutils::to_bytes(test_object, bytes_to_encrypt);
    std::vector<uint8_t> encrypted_bytes = encryptor.make_encrypted_message(bytes_to_encrypt, buffer_size);
    std::cout << "Serialized object of size " << buffer_size << " encrypted to message of size " << encrypted_bytes.size() << std::endl;
    std::cout << "Object bytes: " << std::endl;
    print_byte_array(bytes_to_encrypt, buffer_size);
    std::cout << "Encrypted message bytes: " << std::endl;
    print_byte_array(encrypted_bytes.data(), encrypted_bytes.size());

    openssl::EnvelopeDecryptor decryptor(private_key, openssl::CipherAlgorithm::AES256_CBC);
    // Encrypted body format: encrypted session key, IV, encrypted payload
    std::size_t key_iv_size = decryptor.get_IV_size() + decryptor.get_encrypted_key_size();
    // The plaintext will be no larger than the ciphertext, and possibly smaller
    std::cout << "Key and IV are " << key_iv_size << " bytes, ciphertext is " << (encrypted_bytes.size() - key_iv_size) << " bytes" << std::endl;
    std::vector<uint8_t> decrypted_bytes(encrypted_bytes.size() - key_iv_size);
    decryptor.init(encrypted_bytes.data(), encrypted_bytes.data() + decryptor.get_encrypted_key_size());
    std::size_t bytes_written = decryptor.decrypt_bytes(encrypted_bytes.data() + key_iv_size,
                                                        encrypted_bytes.size() - key_iv_size,
                                                        decrypted_bytes.data());
    bytes_written += decryptor.finalize(decrypted_bytes.data() + bytes_written);
    std::cout << "Decrypted " << bytes_written << " bytes into a buffer of size " << decrypted_bytes.size() << std::endl;
    // Shrink the array to fit
    assert(bytes_written <= decrypted_bytes.size());
    decrypted_bytes.resize(bytes_written);
    std::cout << "Decrypted object bytes: " << std::endl;
    print_byte_array(decrypted_bytes.data(), decrypted_bytes.size());
    auto decrypted_object = mutils::from_bytes<StringObject>(nullptr, decrypted_bytes.data());
    std::cout << "Decrypted object: " << decrypted_object->print() << std::endl;
}

void test_blind_signature(openssl::EnvelopeKey private_key, openssl::EnvelopeKey public_key) {
    openssl::BlindSignatureClient client(public_key);
    StringObject test_object(666, "A value to sign blindly...A value to sign blindly...A value to sign blindly...A value to sign blindly");
    std::size_t buffer_size = mutils::bytes_size(test_object);
    uint8_t serialized_object[buffer_size];
    mutils::to_bytes(test_object, serialized_object);
    std::vector<uint8_t> blind_bytes = client.make_blind_message(serialized_object, buffer_size);
    std::cout << "Serialized object of size " << buffer_size << " blinded to message of size " << blind_bytes.size() << std::endl;
    std::cout << "Object bytes: " << std::endl;
    print_byte_array(serialized_object, buffer_size);
    std::cout << "Blinded message bytes: " << std::endl;
    print_byte_array(blind_bytes.data(), blind_bytes.size());

    openssl::BlindSigner server(private_key);
    std::vector<uint8_t> signed_bytes = server.sign_blinded(blind_bytes.data(), blind_bytes.size());
    std::cout << "Server's blind signature on message, size " << signed_bytes.size() << ": " << std::endl;
    print_byte_array(signed_bytes.data(), signed_bytes.size());
    uint8_t raw_output_signed_bytes[signed_bytes.size()];
    server.sign_blinded(blind_bytes.data(), blind_bytes.size(), raw_output_signed_bytes);
    if(memcmp(signed_bytes.data(), raw_output_signed_bytes, signed_bytes.size()) != 0) {
        std::cout << "ERROR: Signature produced by out-parameter version of sign_blinded did not match. Bytes are:" << std::endl;
        print_byte_array(raw_output_signed_bytes, signed_bytes.size());
    }

    std::vector<uint8_t> signature_bytes = client.unblind_signature(signed_bytes.data(), signed_bytes.size(),
                                                                    serialized_object, buffer_size);
    std::cout << "Unblinded signature bytes, size " << signature_bytes.size() << ": " << std::endl;
    print_byte_array(signature_bytes.data(), signature_bytes.size());

    // The BRSA library uses SHA384 as the default hash method for its RSA signatures
    openssl::Verifier client_verifier(public_key, openssl::DigestAlgorithm::SHA384);
    bool verified = client_verifier.verify_bytes(serialized_object, buffer_size,
                                                 signature_bytes.data(), signature_bytes.size());
    if(verified) {
        std::cout << "Unblinded signature verified successfully!" << std::endl;
    } else {
        std::cout << "ERROR: Unblinded signature did not verify against the object bytes" << std::endl;
        std::cout << "Signature did not verify due to the following error: " << openssl::get_error_string(ERR_get_error(), "") << std::endl;
    }
    bool verified_by_brsa = client.verify_signature(serialized_object, buffer_size,
                                                    signature_bytes.data(), signature_bytes.size());
    if(verified_by_brsa) {
        std::cout << "Unblinded signature verified by BRSA's verify method" << std::endl;
    } else {
        std::cout << "ERROR: Unblinded signature did not verify using BRSA's verify method" << std::endl;
        std::cout << "Verification failed with the following error: " << openssl::get_error_string(ERR_get_error(), "") << std::endl;
    }
}

void test_signature(openssl::EnvelopeKey private_key, openssl::EnvelopeKey public_key) {
    openssl::Signer signer(private_key, openssl::DigestAlgorithm::SHA256);
    signer.init();
    StringObject test_object(98765432, "Test object to sign...test object to sign...");
    std::size_t buffer_size = mutils::bytes_size(test_object);
    uint8_t serialized_object[buffer_size];
    mutils::to_bytes(test_object, serialized_object);
    signer.add_bytes(serialized_object, buffer_size);
    int signature_size = signer.get_max_signature_size();
    uint8_t signature_buffer[signature_size];
    signer.finalize(signature_buffer);
    std::cout << "Serialized object of size " << buffer_size << " got a signature of size " << signature_size << std::endl;
    std::cout << "Signature bytes: " << std::endl;
    print_byte_array(signature_buffer, signature_size);
    uint8_t one_step_signature[signature_size];
    signer.sign_bytes(serialized_object, buffer_size, one_step_signature);
    if(memcmp(signature_buffer, one_step_signature, signature_size) != 0) {
        std::cout << "ERROR: Signature produced by one-step sign_bytes did not match signature produced by add_bytes" << std::endl;
        std::cout << "One-step signature bytes: " << std::endl;
        print_byte_array(one_step_signature, signature_size);
    }

    openssl::Verifier verifier(public_key, openssl::DigestAlgorithm::SHA256);
    verifier.init();
    verifier.add_bytes(serialized_object, buffer_size);
    bool verified = verifier.finalize(signature_buffer, signature_size);
    if(verified) {
        std::cout << "Signature verified successfully!" << std::endl;
    } else {
        std::cout << "ERROR: Signature did not verify against the serialized object" << std::endl;
        std::cout << "Verification failed with the following error: " << openssl::get_error_string(ERR_get_error(), "") << std::endl;
    }
    bool one_step_verified = verifier.verify_bytes(serialized_object, buffer_size, signature_buffer, signature_size);
    if(!one_step_verified) {
        std::cout << "ERROR: One-step verify_bytes failed to verify" << std::endl;
        std::cout << "Verification failed with the following error: " << openssl::get_error_string(ERR_get_error(), "") << std::endl;
    }
}

/**
 * Tests various functions from the OpenSSL wrapper library to make sure I
 * wrote it correctly and it works as expected.
 *
 * Arguments: [private key file] [public key file]
 */
int main(int argc, char** argv) {
    std::string private_key_file;
    std::string public_key_file;
    if(argc > 2) {
        private_key_file = argv[1];
        public_key_file = argv[2];
    } else {
        private_key_file = "private_key.pem";
        public_key_file = "public_key.pem";
    }
    openssl::EnvelopeKey my_private_key = openssl::EnvelopeKey::from_pem_private(private_key_file);
    openssl::EnvelopeKey my_public_key = openssl::EnvelopeKey::from_pem_public(public_key_file);

    test_envelope_encryption(my_private_key, my_public_key);
    test_blind_signature(my_private_key, my_public_key);
    test_signature(my_private_key, my_public_key);
}