#pragma once

#include "adq/util/FixedPoint.hpp"

#include <cstdint>
#include <array>

namespace adq
{

constexpr int RSA_STRENGTH = 2048;
constexpr int RSA_SIGNATURE_SIZE = RSA_STRENGTH / 8;

using SignatureArray = std::array<uint8_t, RSA_SIGNATURE_SIZE>;
using FixedPoint_t = util::FixedPoint<int64_t, 16>;

//All classes are forward declared here, so headers
//can include this header to avoid circular includes
template<typename RecordType>
class NetworkManager;
template<typename RecordType>
class ProtocolState;
class CryptoLibrary;

namespace messaging {
class AggregationMessage;
class AggregationMessageValue;
class OverlayTransportMessage;
class PingMessage;
class QueryRequest;
class SignatureRequest;
class SignatureResponse;
}
}