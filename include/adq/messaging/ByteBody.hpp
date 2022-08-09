#pragma once

#include "MessageBody.hpp"
#include "MessageBodyType.hpp"
#include "adq/mutils-serialization/SerializationSupport.hpp"

#include <cstdint>
#include <ostream>
#include <vector>

namespace adq {
namespace messaging {

/**
 * A simple MessageBody that's just an array of bytes. Used for encrypted message
 * bodies that must be decrypted by CryptoLibrary before they can be deserialized
 * into distinct data types. This wraps a std::vector in an object of type MessageBody
 */
class ByteBody : public MessageBody {
private:
    std::vector<uint8_t> bytes;

public:
    static const constexpr MessageBodyType type = MessageBodyType::BYTES;
    /**
     * Constructs a ByteBody by forwarding constructor arguments to
     * std::vector<uint8_t>
     * @param args Arguments to a constructor of std::vector<uint8_t>
     */
    template <typename... ArgTypes>
    ByteBody(ArgTypes&&... args) : bytes(std::forward<ArgTypes>(args)...) {}
    ByteBody(const ByteBody&) = default;
    ByteBody(ByteBody&&) = default;
    virtual ~ByteBody() = default;

    ByteBody& operator=(const ByteBody& other);
    ByteBody& operator=(ByteBody&& other);
    virtual bool operator==(const MessageBody& rhs) const;

    /**
     * Returns a pointer to the raw byte array within the ByteBody's
     * std::vector of bytes.
     */
    uint8_t* data() noexcept { return bytes.data(); }
    const uint8_t* data() const noexcept { return bytes.data(); }
    /**
     * Returns the size of the byte array
     */
    decltype(bytes)::size_type size() const noexcept { return bytes.size(); }

    /**
     * Resizes the byte array by forwarding arguments to std::vector::resize
     */
    template <typename... A>
    void resize(A&&... args) { return bytes.resize(std::forward<A>(args)...); }

    /**
     * Allow a MessageBody to be used implicitly as a const reference to a
     * std::vector<uint8_t>, which just accesses the underlying vector
     */
    operator const std::vector<uint8_t>&() const { return bytes; }

    friend std::ostream& operator<<(std::ostream& stream, const ByteBody& v);

    std::size_t bytes_size() const;
    std::size_t to_bytes(uint8_t* buffer) const;
    void post_object(const std::function<void(uint8_t const* const, std::size_t)>& f) const;
    static std::unique_ptr<ByteBody> from_bytes(mutils::DeserializationManager* m, uint8_t const* buffer);
};

std::ostream& operator<<(std::ostream& stream, const ByteBody& v);

}  // namespace messaging
}  // namespace adq