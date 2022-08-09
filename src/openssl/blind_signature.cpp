#include "adq/openssl/blind_signature.hpp"
#include "adq/openssl/blind_rsa.h"

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
    // The BRSASecretKey struct just holds an EVP_PKEY* and needs no other initialization
    private_key_for_brsa.evp_pkey = private_key;
}

BlindSigner::~BlindSigner() {
    // Do not deinit private_key_for_brsa since it shares a pointer with private_key
}

std::vector<uint8_t> BlindSigner::sign_blinded(const uint8_t* input_buffer, std::size_t input_size) {
    BRSABlindSignature signature;
    // Even though the BRSABlindMessage struct is input-only and will not be modified,
    // we still have to cast away input_buffer's const in order to initialize it
    const BRSABlindMessage input_as_message{const_cast<uint8_t*>(input_buffer), input_size};
    if(brsa_blind_sign(&context, &signature, &private_key_for_brsa, &input_as_message) != 0) {
        throw blind_signature_error("Failed to blindly sign a message of size " + std::to_string(input_size));
    }
    std::vector<uint8_t> output_buffer(signature.blind_sig_len);
    std::memcpy(output_buffer.data(), signature.blind_sig, signature.blind_sig_len);
    brsa_blind_signature_deinit(&signature);
    return output_buffer;
}

BlindSignatureClient::BlindSignatureClient(const EnvelopeKey& destination_public_key)
    : public_key(destination_public_key) {
    brsa_context_init_default(&context);
    // Copy the public key out to DER format, then copy it back in using brsa_publickey_import, to give BRSA its own copy
    std::unique_ptr<BIO, DeleterFor<BIO>> der_bio(BIO_new(BIO_s_mem()));
    if(i2d_PUBKEY_bio(der_bio.get(), public_key) != 1) {
        throw openssl_error(ERR_get_error(), "Write public key to DER format");
    }
    uint8_t* der_bio_data;
    long data_size = BIO_get_mem_data(der_bio.get(), &der_bio_data);
    if(brsa_publickey_import(&public_key_for_brsa, der_bio_data, data_size) != 0) {
        throw blind_signature_error("Failed to import a public key from DER encoding");
    }
}

BlindSignatureClient::~BlindSignatureClient() {
    brsa_blinding_secret_deinit(&current_blinding_secret);
    brsa_publickey_deinit(&public_key_for_brsa);
}

std::vector<uint8_t> BlindSignatureClient::make_blind_message(const uint8_t* input_buffer, std::size_t input_size) {
    BRSABlindMessage blind_message;
    BRSABlindingSecret blinding_secret;
    if(brsa_blind(&context, &blind_message, &blinding_secret, &public_key_for_brsa, input_buffer, input_size) != 0) {
        throw blind_signature_error("Failed to blind a message of length " + std::to_string(input_size));
    }
    brsa_blinding_secret_deinit(&current_blinding_secret);
    current_blinding_secret = blinding_secret;
    std::vector<uint8_t> output_buffer(blind_message.blind_message_len);
    std::memcpy(output_buffer.data(), blind_message.blind_message, blind_message.blind_message_len);
    return output_buffer;
}

std::vector<uint8_t> BlindSignatureClient::unblind_signature(const uint8_t* blind_signature_buffer, std::size_t signature_size,
                                                             const uint8_t* data_buffer, std::size_t data_size) {
    BRSASignature clear_signature;
    // Even though the BRSABlindSignature struct is input-only and will not be modified,
    // we still have to cast away the input buffer's const in order to initialize it
    const BRSABlindSignature input_as_blind_sig{const_cast<uint8_t*>(blind_signature_buffer), signature_size};
    if(brsa_finalize(&context, &clear_signature, &input_as_blind_sig, &current_blinding_secret,
                     &public_key_for_brsa, data_buffer, data_size) != 0) {
        throw blind_signature_error("Failed to unblind a signature, or signature was not valid");
    }
    std::vector<uint8_t> output_buffer(clear_signature.sig_len);
    std::memcpy(output_buffer.data(), clear_signature.sig, clear_signature.sig_len);
    return output_buffer;
}

}  // namespace openssl