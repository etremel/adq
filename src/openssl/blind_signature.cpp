#include "adq/openssl/blind_signature.hpp"
#include "adq/openssl/blind_rsa.h"
#include "adq/openssl/openssl_exception.hpp"

#include <openssl/bio.h>
#include <openssl/x509.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace openssl {

BlindSigner::BlindSigner(const EnvelopeKey& my_private_key)
    : private_key(my_private_key) {
    brsa_context_init_default(&context);
    // Copy the private key out to DER format, then copy it back in using brsa_secretkey_import, to give BRSA its own copy
    // First call i2d_PrivateKey with NULL to make it return the size of the buffer it wants
    int der_buffer_size = i2d_PrivateKey(private_key, NULL);
    std::vector<uint8_t> der_buffer(der_buffer_size);
    // Make a copy of this pointer because id2_PrivateKey will change it
    uint8_t* der_buffer_ptr = der_buffer.data();
    int bytes_written = i2d_PrivateKey(private_key, &der_buffer_ptr);
    if(bytes_written < 0) {
        throw openssl_error(ERR_get_error(), "Write private key to DER format");
    }
    if(brsa_secretkey_import(&private_key_for_brsa, der_buffer.data(), der_buffer.size()) != 0) {
        throw openssl_error(ERR_get_error(), "Import private key from DER format");
    }
}

BlindSigner::~BlindSigner() {
    brsa_secretkey_deinit(&private_key_for_brsa);
}

std::vector<uint8_t> BlindSigner::sign_blinded(const uint8_t* input_buffer, std::size_t input_size) {
    BRSABlindSignature signature;
    // Even though the BRSABlindMessage struct is input-only and will not be modified,
    // we still have to cast away input_buffer's const in order to initialize it
    const BRSABlindMessage input_as_message{const_cast<uint8_t*>(input_buffer), input_size};
    if(brsa_blind_sign(&context, &signature, &private_key_for_brsa, &input_as_message) != 0) {
        throw openssl_error(ERR_get_error(), "Blindly sign message");
    }
    std::vector<uint8_t> output_buffer(signature.blind_sig_len);
    std::memcpy(output_buffer.data(), signature.blind_sig, signature.blind_sig_len);
    brsa_blind_signature_deinit(&signature);
    return output_buffer;
}

void BlindSigner::sign_blinded(const uint8_t* input_buffer, std::size_t input_size, uint8_t* signature_buffer) {
    BRSABlindSignature signature;
    const BRSABlindMessage input_as_message{const_cast<uint8_t*>(input_buffer), input_size};
    if(brsa_blind_sign(&context, &signature, &private_key_for_brsa, &input_as_message) != 0) {
        throw openssl_error(ERR_get_error(), "Blindly sign message");
    }
    // brsa_blind_sign always allocates new memory for its output, so we have to copy it to the destination buffer
    std::memcpy(signature_buffer, signature.blind_sig, signature.blind_sig_len);
    brsa_blind_signature_deinit(&signature);
}

BlindSignatureClient::BlindSignatureClient(const EnvelopeKey& destination_public_key)
    : public_key(destination_public_key),
      has_current_blinding_secret(false) {
    brsa_context_init_default(&context);
    // Copy the public key out to DER format, then copy it back in using brsa_publickey_import, to give BRSA its own copy
    // First call i2d_PublicKey with NULL to make it return the size of the buffer it wants
    int der_buffer_size = i2d_PublicKey(public_key, NULL);
    std::vector<uint8_t> der_buffer(der_buffer_size);
    // Make a copy of this pointer because id2_PublicKey will change it
    uint8_t* der_buffer_ptr = der_buffer.data();
    int bytes_written = i2d_PublicKey(public_key, &der_buffer_ptr);
    if(bytes_written < 0) {
        throw openssl_error(ERR_get_error(), "Write public key to DER format");
    }
    if(brsa_publickey_import(&public_key_for_brsa, der_buffer.data(), der_buffer.size()) != 0) {
        throw openssl_error(ERR_get_error(), "Import public key from DER format");
    }
}

BlindSignatureClient::~BlindSignatureClient() {
    if(has_current_blinding_secret) {
        brsa_blinding_secret_deinit(&current_blinding_secret);
    }
    brsa_publickey_deinit(&public_key_for_brsa);
}

std::vector<uint8_t> BlindSignatureClient::make_blind_message(const uint8_t* input_buffer, std::size_t input_size) {
    BRSABlindMessage blind_message;
    BRSABlindingSecret blinding_secret;
    if(brsa_blind(&context, &blind_message, &blinding_secret, &public_key_for_brsa, input_buffer, input_size) != 0) {
        throw openssl_error(ERR_get_error(), "Blind a message");
    }
    if(has_current_blinding_secret) {
        brsa_blinding_secret_deinit(&current_blinding_secret);
    }
    current_blinding_secret = blinding_secret;
    has_current_blinding_secret = true;
    std::vector<uint8_t> output_buffer(blind_message.blind_message_len);
    std::memcpy(output_buffer.data(), blind_message.blind_message, blind_message.blind_message_len);
    brsa_blind_message_deinit(&blind_message);
    return output_buffer;
}

std::vector<uint8_t> BlindSignatureClient::unblind_signature(const uint8_t* blind_signature_buffer, std::size_t blind_signature_size,
                                                             const uint8_t* data_buffer, std::size_t data_size) {
    if(!has_current_blinding_secret) {
        throw blind_signature_error("unblind_signature called without a current blinding secret. make_blind_message must be called first.");
    }
    BRSASignature clear_signature;
    // Even though the BRSABlindSignature struct is input-only and will not be modified,
    // we still have to cast away the input buffer's const in order to initialize it
    const BRSABlindSignature input_as_blind_sig{const_cast<uint8_t*>(blind_signature_buffer), blind_signature_size};
    if(brsa_finalize(&context, &clear_signature, &input_as_blind_sig, &current_blinding_secret,
                     &public_key_for_brsa, data_buffer, data_size) != 0) {
        throw blind_signature_error("Failed to unblind a signature, or signature was not valid");
    }
    std::vector<uint8_t> output_buffer(clear_signature.sig_len);
    std::memcpy(output_buffer.data(), clear_signature.sig, clear_signature.sig_len);
    brsa_signature_deinit(&clear_signature);
    return output_buffer;
}

bool BlindSignatureClient::verify_signature(const uint8_t* data_buffer, std::size_t data_size,
                                            const uint8_t* signature_buffer, std::size_t signature_size) {
    BRSASignature input_as_brsa_signature{const_cast<uint8_t*>(signature_buffer), signature_size};
    if(brsa_verify(&context, &input_as_brsa_signature, &public_key_for_brsa, data_buffer, data_size) == 0) {
        return true;
    } else {
        return false;
    }
}

}  // namespace openssl