#pragma once

#include <spdlog/spdlog.h>

#include <adq/core/ProtocolState.hpp>
#include <memory>

namespace adq {
namespace messaging {
class AggregationMessage;
class OverlayTransportMessage;
class PingMessage;
class QueryRequest;
class SignatureResponse;
}  // namespace messaging
}  // namespace adq

namespace adq {

class QueryClient {
public:
    /** The ID of this client device in the query network. */
    const int my_id;
    /** The total number of devices in the network. */
    int num_clients;

private:
    std::shared_ptr<spdlog::logger> logger;
    ProtocolState query_protocol_state;

public:
    QueryClient(int id, int num_clients);
};

}  // namespace adq
