#include "../QueryClient.hpp"
#include "adq/config/Configuration.hpp"
#include "adq/core/DataSource.hpp"
#include "adq/core/ProtocolState.hpp"
#include "adq/util/Overlay.hpp"

#include "adq/messaging/AggregationMessage.hpp"
#include "adq/messaging/OverlayMessage.hpp"
#include "adq/messaging/OverlayTransportMessage.hpp"
#include "adq/messaging/PingMessage.hpp"
#include "adq/messaging/QueryRequest.hpp"

#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>

#include <memory>

namespace adq {

template <typename RecordType>
QueryClient<RecordType>::QueryClient(int num_clients,
                                     std::unique_ptr<DataSource<RecordType>> data_source)
    : my_id(Configuration::getInt32(Configuration::SECTION_SETUP, Configuration::CLIENT_ID)),
      num_clients(num_clients),
      logger(spdlog::get("global_logger")),
      network_manager(this),
      query_protocol_state(num_clients, my_id, network_manager, *data_source),
      data_source(std::move(data_source)) {}

template <typename RecordType>
void QueryClient<RecordType>::handle_message(std::shared_ptr<messaging::QueryRequest<RecordType>> message) {
    // Forward the serialized function call to the DataSource object
    RecordType data_to_contribute = data_source->select_functions.at(message->select_function_opcode)(message->select_serialized_args.data());
    bool should_contribute = data_source->filter_functions.at(message->filter_function_opcode)(data_to_contribute, message->filter_serialized_args.data());
    if(should_contribute) {
        query_protocol_state.start_query(message, data_to_contribute);
    }
}

template <typename RecordType>
void QueryClient<RecordType>::handle_message(std::shared_ptr<messaging::PingMessage<RecordType>> message) {
    query_protocol_state.handle_ping_message(*message);
}

template <typename RecordType>
void QueryClient<RecordType>::handle_message(std::shared_ptr<messaging::OverlayTransportMessage<RecordType>> message) {
    if(util::gossip_target(message->sender_id, message->sender_round, num_clients) == my_id) {
        std::shared_ptr<messaging::OverlayMessage<RecordType>> wrapped_message = message->get_body();
        if(wrapped_message->query_num > query_protocol_state.get_current_query_num()) {
            // If the message is for a future query, buffer it until I get the query-start message
            query_protocol_state.buffer_future_message(message);
        } else if(wrapped_message->query_num < query_protocol_state.get_current_query_num()) {
            logger->warn("Client {} discarded an obsolete message from client {} for an old query: {}", my_id, message->sender_id, *message);
            // At this point, we know the message is for the current query
        } else if(message->sender_round == query_protocol_state.get_current_overlay_round()) {
            query_protocol_state.handle_overlay_message(*message);
        } else if(message->sender_round > query_protocol_state.get_current_overlay_round()) {
            // If it's a message for a future round, buffer it until my round advances
            query_protocol_state.buffer_future_message(message);
        } else {
            logger->debug("Client {}, already in round {}, rejected a message from client {} as too old: {}", my_id, query_protocol_state.get_current_overlay_round(), message->sender_id, *message);
        }
    } else {
        logger->warn("Client {} rejected a message because it has the wrong gossip target: {}", my_id, *message);
    }
}

template <typename RecordType>
void QueryClient<RecordType>::handle_message(std::shared_ptr<messaging::AggregationMessage<RecordType>> message) {
    if(util::aggregation_group_for(message->sender_id, query_protocol_state.get_num_aggregation_groups(), num_clients) ==
       util::aggregation_group_for(my_id, query_protocol_state.get_num_aggregation_groups(), num_clients)) {
        if(query_protocol_state.is_in_aggregate_phase()) {
            query_protocol_state.handle_aggregation_message(*message);
            // If it's a message for the right query, but I received it too early, buffer it for the future
        } else if(message->query_num == query_protocol_state.get_current_query_num()) {
            query_protocol_state.buffer_future_message(message);
        } else {
            logger->warn("Meter {} rejected a message from meter {} with the wrong query number: {}", my_id, message->sender_id, *message);
        }
    }
}
template <typename RecordType>
void QueryClient<RecordType>::handle_message(std::shared_ptr<messaging::SignatureResponse<RecordType>> message) {
    query_protocol_state.handle_signature_response(*message);
}

template <typename RecordType>
void QueryClient<RecordType>::handle_message(std::shared_ptr<messaging::SignatureRequest<RecordType>> message) {
    logger->warn("Client {} received a signature request message, which can only be handled by a server. Ignoring it.", my_id);
}

template <typename RecordType>
void QueryClient<RecordType>::main_loop() {
    network_manager.run();
}

}  // namespace adq
