#pragma once

#include "adq/util/FixedPoint.hpp"

#include <array>
#include <cstdint>

namespace adq {

constexpr int RSA_STRENGTH = 2048;
constexpr int RSA_SIGNATURE_SIZE = RSA_STRENGTH / 8;

using SignatureArray = std::array<uint8_t, RSA_SIGNATURE_SIZE>;
using FixedPoint_t = util::FixedPoint<int64_t, 16>;

// All classes are forward declared here, so headers
// can include this header to avoid circular includes
template <typename RecordType>
class NetworkManager;
template <typename RecordType>
class ProtocolState;
template <typename RecordType>
class QueryClient;

class CryptoLibrary;

namespace messaging {
template <typename RecordType>
class AggregationMessage;
template <typename RecordType>
class AggregationMessageValue;
class OverlayTransportMessage;
class PingMessage;
class QueryRequest;
class SignatureRequest;
class SignatureResponse;
}  // namespace messaging
}  // namespace adq