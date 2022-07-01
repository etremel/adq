#include "../QueryClient.hpp"
#include "adq/core/DataSource.hpp"
#include "adq/core/ProtocolState.hpp"
#include "adq/util/Overlay.hpp"

#include "adq/messaging/AggregationMessage.hpp"
#include "adq/messaging/OverlayMessage.hpp"
#include "adq/messaging/OverlayTransportMessage.hpp"
#include "adq/messaging/PingMessage.hpp"
#include "adq/messaging/QueryRequest.hpp"

#include <spdlog/spdlog.h>

#include <memory>

namespace adq {

template <typename RecordType>
QueryClient<RecordType>::QueryClient(int id,
                                     int num_clients,
                                     uint16_t service_port,
                                     const std::map<int, asio::ip::tcp::endpoint>& client_id_to_ip_map,
                                     const std::string& private_key_filename,
                                     const std::map<int, std::string>& public_key_files_by_id,
                                     std::unique_ptr<DataSource<RecordType>> data_source)
    : my_id(id),
      num_clients(num_clients),
      network_manager(*this, service_port, client_id_to_ip_map),
      query_protocol_state(num_clients, id, network_manager, private_key_filename, public_key_files_by_id),
      data_source(std::move(data_source)) {}

template <typename RecordType>
void QueryClient<RecordType>::handle_message(std::shared_ptr<messaging::QueryRequest> message) {
    // Forward the serialized function call to the DataSource object
    RecordType data_to_contribute = data_source->select_functions.at(message->select_function_opcode)(message->select_serialized_args);
    bool should_contribute = data_source->filter_functions.at(message->filter_function_opcode)(data_to_contribute, message->filter_serialized_args);
    if(should_contribute) {
        query_protocol_state.start_query(message, data_to_contribute);
    }
}

template <typename RecordType>
void QueryClient<RecordType>::handle_message(std::shared_ptr<messaging::PingMessage> message) {
    query_protocol_state.handle_ping_message(message);
}

template <typename RecordType>
void QueryClient<RecordType>::handle_message(std::shared_ptr<messaging::OverlayTransportMessage> message) {
    if(util::gossip_target(message->sender_id, message->sender_round, num_clients) == my_id) {
        std::shared_ptr<messaging::OverlayMessage> wrapped_message = std::static_pointer_cast<messaging::OverlayMessage>(message->body);
        if(wrapped_message->query_num > query_protocol_state.get_current_query_num()) {
            // If the message is for a future query, buffer it until I get the query-start message
            query_protocol_state.buffer_future_message(message);
        } else if(wrapped_message->query_num < query_protocol_state.get_current_query_num()) {
            logger->warn("Client {} discarded an obsolete message from client {} for an old query: {}", my_id, message->sender_id, *message);
            // At this point, we know the message is for the current query
        } else if(message->sender_round == query_protocol_state.get_current_overlay_round()) {
            query_protocol_state.handle_overlay_message(message);
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
            query_protocol_state.handle_aggregation_message(message);
            // If it's a message for the right query, but I received it too early, buffer it for the future
        } else if(message->query_num == query_protocol_state.get_current_query_num()) {
            query_protocol_state.buffer_future_message(message);
        } else {
            logger->warn("Meter {} rejected a message from meter {} with the wrong query number: {}", my_id, message->sender_id, *message);
        }
    }
}
template <typename RecordType>
void QueryClient<RecordType>::handle_message(std::shared_ptr<messaging::SignatureResponse> message) {
    query_protocol_state.handle_signature_response(message);
}

template <typename RecordType>
void QueryClient<RecordType>::main_loop() {
    network_manager.run();
}

}  // namespace adq