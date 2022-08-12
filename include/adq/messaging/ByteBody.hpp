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
 *
 * @tparam RecordType The type of data being collected by queries. This parameter
 * is ignored by ByteBody and has no effect, but it's required in order to inherit
 * from MessageBody<RecordType>
 */
template <typename RecordType>
class ByteBody : public MessageBody<RecordType> {
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
    virtual bool operator==(const MessageBody<RecordType>& rhs) const;

    /**
     * Returns a pointer to the raw byte array within the ByteBody's
     * std::vector of bytes.
     */
    uint8_t* data() noexcept { return bytes.data(); }
    const uint8_t* data() const noexcept { return bytes.data(); }
    /**
     * Returns the size of the byte array
     */
    typename decltype(bytes)::size_type size() const noexcept { return bytes.size(); }

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

    // Note: friend functions of templates must be defined inline
    friend std::ostream& operator<<(std::ostream& stream, const ByteBody& v) {
        if(v.size() > 0) {
            // Print the internal vector in hex format
            std::ios normal_stream_state(nullptr);
            normal_stream_state.copyfmt(stream);
            stream << '[' << std::hex << std::setfill('0') << std::setw(2) << std::right;
            std::copy(v.bytes.begin(), v.bytes.end(), std::ostream_iterator<uint8_t>(stream, ", "));
            stream << "\b\b]";
            stream.copyfmt(normal_stream_state);
        }
        return stream;
    }

    std::size_t bytes_size() const;
    std::size_t to_bytes(uint8_t* buffer) const;
    void post_object(const std::function<void(uint8_t const* const, std::size_t)>& f) const;
    static std::unique_ptr<ByteBody<RecordType>> from_bytes(mutils::DeserializationManager* m, uint8_t const* buffer);
};

// template <typename RecordType>
// std::ostream& operator<<(std::ostream& stream, const ByteBody<RecordType>& v);

}  // namespace messaging
}  // namespace adq

#include "detail/ByteBody_impl.hpp"