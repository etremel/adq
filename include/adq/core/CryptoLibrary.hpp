#pragma once

#include "InternalTypes.hpp"
#include "adq/messaging/OverlayMessage.hpp"
#include "adq/messaging/SignedValue.hpp"
#include "adq/messaging/StringBody.hpp"
#include "adq/messaging/ValueContribution.hpp"
#include "adq/messaging/ValueTuple.hpp"
#include "adq/openssl/signature.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <string>

namespace adq {

/**
 * Contains all the cryptography functions needed by the query protocols,
 * encapsulating the details of exactly which cryptography library is used
 * to implement tools like signatures and public-key encryption. Currently
 * we use OpenSSL.
 */
class CryptoLibrary {
private:
    openssl::EnvelopeKey my_private_key;
    std::map<int, openssl::EnvelopeKey> public_keys_by_id;
    openssl::Signer my_signer;

public:
    /**
     * Constructs a CryptoLibrary, loading the local client's private key and the
     * other clients' public keys from files on disk.
     *
     * @param private_key_filename The name of the file containing this node's private key.
     * @param public_key_files_by_id Maps each node ID to the name of the PEM
     * file containing that node's public key. The server's public key should
     * be at the entry for -1.
     */
    CryptoLibrary(const std::string& private_key_filename, const std::map<int, std::string>& public_key_files_by_id);

    /**
     * Encrypts the body of an OverlayMessage under the public key of the given client.
     * @param message The message to encrypt; after calling this method, its body will be encrypted
     * @param target_id The ID of the client whose public key should be used to encrypt
     */
    void rsa_encrypt(messaging::OverlayMessage& message, const int target_id);

    /**
     * Decrypts the body of an encrypted OverlayMessage, using the private
     * key of the current client.
     * @param message An OverlayMessage with an encrypted body; after calling
     * this method, its body will be decrypted
     */
    void rsa_decrypt(messaging::OverlayMessage& message);

    /**
     * Creates a blinded message representing a ValueTuple, by multiplying
     * its numeric representation by a random value that is invertible under
     * the RSA modulus of utility's public key. (Blinded messages are only ever
     * sent to the utility.)
     * @param value The value to blind
     * @return A byte sequence containing the blinded message.
     */
    template <typename RecordType>
    std::shared_ptr<messaging::StringBody> rsa_blind(const messaging::ValueTuple<RecordType>& value);

    /**
     * Signs a blinded message with the current client's private key.
     * This should not be used to sign any other kind of message, because
     * it uses a nonstandard "raw" signature.
     * @param encrypted_message A blinded message to sign, which will be
     * stored as a sequence of bytes and sent in an OverlayMessage (hence
     * the StringBody type).
     * @return A blind signature over the message, which is also a
     * sequence of bytes represented as a StringBody
     */
    std::shared_ptr<messaging::StringBody> rsa_sign_blinded(const messaging::StringBody& blinded_message);

    /**
     * Unblinds a signature using the inverse of the blinding value this
     * client most recently used. This only works because blind-signature
     * requests are sent sequentially (and to only one destination,
     * the utility). The unblinded signature is placed in the SignatureArray
     * parameter.
     * @param blinded_signature The blinded signature to unblind
     * @param signature The unblinded signature
     */
    void rsa_unblind_signature(const std::string& blinded_signature,
                               SignatureArray& signature);

    /**
     * Signs a ciphertext with the current client's private key.
     * @param encrypted_message The ciphertext to sign, which we expect to
     * be the encrypted body of an OverlayMessage (hence the StringBody type).
     * @return A blind signature over the ciphertext, which is also a
     * ciphertext represented as a StringBody
     */
    std::shared_ptr<messaging::StringBody> rsa_sign_encrypted(const messaging::StringBody& encrypted_message);

    /**
     * Decrypts a ciphertext with the current client's private key, assuming
     * the ciphertext is a blinded signature, and places the resulting
     * signature in the SignatureArray.
     * @param blinded_signature The blinded signature to decrypt
     * @param signature The decrypted signature
     */
    void rsa_decrypt_signature(const std::string& blinded_signature,
                               SignatureArray& signature);

    /**
     * Encrypts a ValueTuple under under the public key of the given client.
     * @param value The ValueTuple to encrypt
     * @param target_id The ID of the client whose public key should
     * be used to encrypt the ValueTuple
     * @return A string representing the bits of the ciphertext (as a
     * StringBody since it will probably be sent in a message)
     */
    template <typename RecordType>
    std::shared_ptr<messaging::StringBody> rsa_encrypt(const messaging::ValueTuple<RecordType>& value,
                                                       const int target_id);

    /**
     * Signs a ValueContribution with the current client's private key, and
     * places the resulting signature in the SignatureArray
     * @param value The ValueContribution to sign
     * @param signature The signature over the ValueContribution
     */
    template <typename RecordType>
    void rsa_sign(const messaging::ValueContribution<RecordType>& value, SignatureArray& signature);

    /**
     * Verifies the signature on a ValueContribution against the public key
     * of the client with the given ID.
     * @param value The ValueContribution that the signature covers
     * @param signature The signature on the ValueContribution
     * @param signer_id The ID of the client that supposedly produced
     * the signature
     * @return True if the signature is valid, false if it is not
     */
    template <typename RecordType>
    bool rsa_verify(const messaging::ValueContribution<RecordType>& value,
                    const SignatureArray& signature, const int signer_id);

    /**
     * Signs a SignedValue with the current client's private key, and places
     * the resulting signature in the SignatureArray
     * @param value The SignedValue to sign
     * @param signature The signature over the SignedValue
     */
    template <typename RecordType>
    void rsa_sign(const messaging::SignedValue<RecordType>& value, SignatureArray& signature);

    /**
     * Verifies the signature on a SignedValue against the public key of
     * the meter with the given ID.
     * @param value The SignedValue that the signature covers
     * @param signature The signature on the SignedValue
     * @param signer_meter_id The ID of the meter that supposedly produced
     * the signature
     * @return True if the signature is valid, false if it is not
     */
    template <typename RecordType>
    bool rsa_verify(const messaging::SignedValue<RecordType>& value, const SignatureArray& signature,
                    const int signer_meter_id);
};

}  // namespace adq

#include "detail/CryptoLibrary_impl.hpp"