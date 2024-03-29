#pragma once

#include "blind_rsa.h"
#include "envelope_key.hpp"
#include "openssl_exception.hpp"
#include "pointers.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace openssl {

/**
 * An exception that indicates an error in the Blind RSA Signatures library
 * that was not caused directly by an OpenSSL function. This library doesn't
 * extend the OpenSSL error-code system, so we can't always use openssl_error.
 */
struct blind_signature_error : public std::runtime_error {
    blind_signature_error(const std::string& message) : runtime_error(message) {}
};

class BlindSigner {
    EnvelopeKey private_key;
    BRSASecretKey private_key_for_brsa;
    BRSAContext context;

public:
    BlindSigner(const EnvelopeKey& my_private_key);
    ~BlindSigner();
    /**
     * Signs a blinded message using this signer's configured private key and returns
     * the blind signature in a new byte buffer.
     *
     * @param input_buffer A pointer to a byte array containing the message to sign.
     * @param input_size The size of the input byte array
     * @return A byte array containing the blind signature
     */
    std::vector<uint8_t> sign_blinded(const uint8_t* input_buffer, std::size_t input_size);
    /**
     * Signs a blinded message using this signer's configured private key and places
     * the signature in the buffer pointed to by signature_buffer.
     *
     * @param input_buffer A pointer to a byte array containing the message to sign.
     * @param input_size The size of the input byte array
     * @param signature_buffer A pointer to a byte array in which the blind signature
     * should be written. Must be large enough to contain the signature.
     */
    void sign_blinded(const uint8_t* input_buffer, std::size_t input_size, uint8_t* signature_buffer);
};

class BlindSignatureClient {
    EnvelopeKey public_key;
    BRSAPublicKey public_key_for_brsa;
    /** True if current_blinding_secret has a value, false if it is uninitialized */
    bool has_current_blinding_secret;
    BRSABlindingSecret current_blinding_secret;
    BRSAContext context;

public:
    BlindSignatureClient(const EnvelopeKey& destination_public_key);
    ~BlindSignatureClient();
    /**
     * Creates a blinded version of the bytes in the input buffer, and saves the
     * blinding secret internally so it can be used to unblind a signature on
     * these bytes.
     *
     * @param input_buffer A pointer to a byte array containing some data that needs
     * to be blindly signed
     * @param input_size The size of the input byte array
     * @return A byte array containing a blinded version of the input array
     */
    std::vector<uint8_t> make_blind_message(const uint8_t* input_buffer, std::size_t input_size);
    /**
     * Unblinds a signature using the blinding secret saved from the most recent
     * call to make_blind_message() and the public key configured with this
     * BlindSignatureClient, and validates it against the data it signs. This
     * will throw a blind_signature_error if the unblinded signature is not valid
     * for the data.
     *
     * @param blind_signature_buffer A pointer to a byte array containing a blind signature.
     * @param blind_signature_size The size of the signature buffer
     * @param data_buffer A pointer to a byte array containing the original data (message)
     * that was blindly signed
     * @param data_size The size of the data buffer
     * @return A byte array containing the unblinded signature
     */
    std::vector<uint8_t> unblind_signature(const uint8_t* blind_signature_buffer, std::size_t blind_signature_size,
                                           const uint8_t* data_buffer, std::size_t data_size);

    /**
     * Verifies a (non-blinded) signature against a data buffer, using the public
     * key configured with this BlindSignatureClient. This must be used instead of
     * the standard openssl::Verifier to verify signatures that were originally
     * blinded, because blinded signatures don't use standard padding.
     *
     * @param data_buffer A pointer to a byte array containing some data that was
     * blindly signed
     * @param data_size The size of the data byte array
     * @param signature_buffer A pointer to a byte array containing a formerly
     * blinded signature on this data
     * @param signature_size The size of the signature byte array
     * @return True if the signature is valid for the data, false if it is not
     */
    bool verify_signature(const uint8_t* data_buffer, std::size_t data_size,
                          const uint8_t* signature_buffer, std::size_t signature_size);
};

}  // namespace openssl