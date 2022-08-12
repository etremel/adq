#pragma once

#include "ByteBody.hpp"
#include "Message.hpp"
#include "MessageType.hpp"

#include <cstdint>
#include <memory>
#include <ostream>

namespace adq {
namespace messaging {

/**
 * Trivial subclass of Message that specializes its body to be a ByteBody.
 *
 * @tparam RecordType The data type being collected in queries in this system.
 * This parameter is ignored by SignatureResponse and has no effect, but it's
 * required in order to inherit from Message<RecordType>
 */
template<typename RecordType>
class SignatureResponse : public Message<RecordType> {
public:
    static const constexpr MessageType type = MessageType::SIGNATURE_RESPONSE;
    using body_type = ByteBody<RecordType>;
    using Message<RecordType>::body;

    SignatureResponse(const int sender_id, std::shared_ptr<ByteBody<RecordType>> encrypted_response)
        : Message<RecordType>(sender_id, std::move(encrypted_response)){};
    virtual ~SignatureResponse() = default;

    /** Returns a pointer to the message body cast to the correct type for this message */
    std::shared_ptr<body_type> get_body() { return std::static_pointer_cast<body_type>(body); };
    const std::shared_ptr<body_type> get_body() const { return std::static_pointer_cast<body_type>(body); };

    std::size_t bytes_size() const;
    std::size_t to_bytes(uint8_t* buffer) const;
    void post_object(const std::function<void(const uint8_t* const, std::size_t)>& function) const;
    static std::unique_ptr<SignatureResponse<RecordType>> from_bytes(mutils::DeserializationManager* m, uint8_t const* buffer);
};

template<typename RecordType>
std::ostream& operator<<(std::ostream& out, const SignatureResponse<RecordType>& message);

} /* namespace messaging */
}  // namespace adq

#include "detail/SignatureResponse_impl.hpp"