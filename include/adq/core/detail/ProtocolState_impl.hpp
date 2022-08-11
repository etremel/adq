#include "../ProtocolState.hpp"

#include "adq/core/CrusaderAgreementState.hpp"
#include "adq/core/CryptoLibrary.hpp"
#include "adq/core/NetworkManager.hpp"
#include "adq/core/TreeAggregationState.hpp"
#include "adq/messaging/ByteBody.hpp"
#include "adq/messaging/PathOverlayMessage.hpp"
#include "adq/messaging/PingMessage.hpp"
#include "adq/messaging/QueryRequest.hpp"
#include "adq/messaging/SignatureRequest.hpp"
#include "adq/messaging/SignatureResponse.hpp"
#include "adq/messaging/SignedValue.hpp"
#include "adq/messaging/ValueContribution.hpp"
#include "adq/messaging/ValueTuple.hpp"
#include "adq/util/LinuxTimerManager.hpp"
#include "adq/util/PathFinder.hpp"

#include <spdlog/spdlog.h>

#include <memory>
#include <vector>

namespace adq {

template <typename RecordType>
ProtocolState<RecordType>::ProtocolState(int num_clients, int local_client_id, NetworkManager<RecordType>& network_manager,
                                         DataSource<RecordType>& data_source, const std::string& private_key_filename,
                                         const std::map<int, std::string>& public_key_files_by_id)
    : logger(spdlog::get("global_logger")),
      protocol_phase(ProtocolPhase::IDLE),
      meter_id(local_client_id),
      num_meters(num_clients),
      log2n((int)std::ceil(std::log2(num_meters))),
      num_aggregation_groups(2 * FAILURES_TOLERATED + 1),
      overlay_round(-1),
      is_last_round(false),
      round_timeout_timer(-1),
      ping_response_from_predecessor(false),
      timers(std::make_unique<util::LinuxTimerManager>()),
      network(network_manager),
      data_source(data_source),
      crypto(private_key_filename, public_key_files_by_id),
      agreement_start_round(0) {}

template <typename RecordType>
void ProtocolState<RecordType>::start_query(std::shared_ptr<messaging::QueryRequest> query_request, const RecordType& contributed_data) {
    overlay_round = -1;
    is_last_round = false;
    ping_response_from_predecessor = false;
    timers->cancel_timer(round_timeout_timer);
    proxy_values.clear();
    failed_meter_ids.clear();
    aggregation_phase_state = std::make_unique<TreeAggregationState<RecordType>>(meter_id, num_aggregation_groups, num_meters,
                                                                                 network, query_request);
    std::vector<int> proxies = util::pick_proxies(meter_id, num_aggregation_groups, num_meters);
    logger->trace("Client {} chose these proxies: {}", meter_id, proxies);
    my_contribution = std::make_shared<messaging::ValueTuple<RecordType>>(query_request->query_number, contributed_data, proxies);

    protocol_phase = ProtocolPhase::SETUP;
    accepted_proxy_values.clear();
    agreement_phase_state = std::make_unique<CrusaderAgreementState<RecordType>>(meter_id, num_meters, query_request->query_number, crypto);
    // Blind my ValueTuple and send it to the utility to be signed
    auto blinded_contribution = crypto.rsa_blind(*my_contribution);
    network.send(std::make_shared<messaging::SignatureRequest>(meter_id, blinded_contribution));
}

template <typename RecordType>
void ProtocolState<RecordType>::handle_signature_response(messaging::SignatureResponse& message) {
    auto signed_contribution = std::make_shared<messaging::ValueContribution<RecordType>>(*my_contribution);
    // Decrypt the utility's signature and copy it into ValueContribution's signature field
    crypto.rsa_unblind_signature(*my_contribution,
                                 *std::static_pointer_cast<messaging::SignatureResponse::body_type>(message.body),
                                 signed_contribution->signature);

    logger->debug("Client {} is finished with Setup", meter_id);
    protocol_phase = ProtocolPhase::SHUFFLE;
    encrypted_multicast_to_proxies(signed_contribution);
}

template <typename RecordType>
void ProtocolState<RecordType>::handle_ping_message(const messaging::PingMessage& message) {
    if(!message.is_response) {
        // If this is a ping request, send a response back
        auto reply = std::make_shared<messaging::PingMessage>(meter_id, true);
        logger->trace("Meter {} replying to a ping from {}", meter_id, message.sender_id);
        network.send(reply, message.sender_id);
    } else if(message.sender_id == util::gossip_predecessor(meter_id, overlay_round, num_meters)) {
        // If this is a ping response and we still care about it
        //(the sender is our predecessor), take note
        ping_response_from_predecessor = true;
    }
}

template <typename RecordType>
void ProtocolState<RecordType>::handle_overlay_message(messaging::OverlayTransportMessage& message) {
    if(is_in_overlay_phase()) {
        timers->cancel_timer(round_timeout_timer);
        round_timeout_timer = timers->register_timer(OVERLAY_ROUND_TIMEOUT, [this]() { handle_round_timeout(); });
    }
    // The only valid MessageBody for an OverlayTransportMessage is an OverlayMessage
    auto overlay_message = std::static_pointer_cast<messaging::OverlayMessage>(message.body);
    if(overlay_message->is_encrypted) {
        // Decrypt the body in-place
        crypto.rsa_decrypt(*overlay_message);
    }
    // If the body is a non-encrypted PathOverlayMessage, add it to waiting_messages
    if(auto path_overlay_message = std::dynamic_pointer_cast<messaging::PathOverlayMessage>(message.body)) {
        if(!path_overlay_message->remaining_path.empty()) {
            // Pop remaining_path into destination and add to waiting_messages
            path_overlay_message->destination = path_overlay_message->remaining_path.front();
            path_overlay_message->remaining_path.pop_front();
            waiting_messages.emplace_back(path_overlay_message);
        }
    }
    // Dummy messages will have a null payload
    if(overlay_message->body != nullptr) {
        /* If it's an encrypted onion that needs to be forwarded, the payload will be the next layer.
         * If the payload is not an OverlayMessage, it's either a PathOverlayMessage or the last layer
         * of the onion. The last layer of the onion will always have destination == meter_id (because
         * it was just received here), but a PathOverlayMessage that still needs to be forwarded will
         * have its destination already set to the next hop by the superclass handle_overlay_message.
         */
        if(auto enclosed_message = std::dynamic_pointer_cast<messaging::OverlayMessage>(overlay_message->body)) {
            waiting_messages.emplace_back(enclosed_message);
        } else if(overlay_message->destination == meter_id) {
            if(protocol_phase == ProtocolPhase::SHUFFLE) {
                handle_shuffle_phase_message(*overlay_message);
            } else if(protocol_phase == ProtocolPhase::AGREEMENT) {
                handle_agreement_phase_message(*overlay_message);
            }
        }  // If destination didn't match, it was already added to waiting_messages
    }
    if(message.is_final_message && is_in_overlay_phase()) {
        end_overlay_round();
    }
}

template <typename RecordType>
void ProtocolState<RecordType>::handle_shuffle_phase_message(const messaging::OverlayMessage& message) {
    // Drop messages that are received in the wrong phase (i.e. not ValueContributions) or have the wrong round number
    if(auto contribution = std::dynamic_pointer_cast<messaging::ValueContribution<RecordType>>(message.body)) {
        if(contribution->value.query_num == my_contribution->query_num) {
            // Verify the owner's signature
            if(crypto.rsa_verify(contribution->value, contribution->signature)) {
                proxy_values.emplace(contribution);
                logger->trace("Meter {} received proxy value: {}", meter_id, *contribution);
            }
        } else {
            logger->warn("Meter {} rejected a proxy value because it had the wrong query number: {}", meter_id, *contribution);
        }
    } else if(message.body != nullptr) {
        logger->warn("Meter {} rejected a message because it was not a ValueContribution: {}", meter_id, message);
    }
}

template <typename RecordType>
void ProtocolState<RecordType>::handle_agreement_phase_message(const messaging::OverlayMessage& message) {
    agreement_phase_state->handle_message(message);
}

template <typename RecordType>
void ProtocolState<RecordType>::handle_aggregation_message(messaging::AggregationMessage<RecordType>& message) {
    aggregation_phase_state->handle_message(message, data_source);
    send_aggregate_if_done();
}

template <typename RecordType>
void ProtocolState<RecordType>::start_aggregate_phase() {
    // Since we're now done with the overlay, stop the timeout waiting for the next round
    timers->cancel_timer(round_timeout_timer);
    // Initialize aggregation helper
    aggregation_phase_state->initialize(failed_meter_ids);
    // If this node is a leaf, aggregation might be done already
    send_aggregate_if_done();
    // If not done already, check future messages for aggregation messages already received from children
    if(is_in_aggregate_phase()) {
        for(auto message_iter = future_aggregation_messages.begin();
            message_iter != future_aggregation_messages.end();) {
            handle_aggregation_message(**message_iter);
            message_iter = future_aggregation_messages.erase(message_iter);
        }
    }
    // Set this because we're done with the overlay
    is_last_round = true;
}

template <typename RecordType>
void ProtocolState<RecordType>::send_aggregate_if_done() {
    if(aggregation_phase_state->done_receiving_from_children()) {
        aggregation_phase_state->compute_and_send_aggregate(accepted_proxy_values, data_source);
        protocol_phase = ProtocolPhase::IDLE;
        logger->debug("Meter {} is finished with Aggregate", meter_id);
    }
}

template <typename RecordType>
void ProtocolState<RecordType>::encrypted_multicast_to_proxies(const std::shared_ptr<messaging::ValueContribution<RecordType>>& contribution) {
    // Find independent paths starting at round 0
    auto proxy_paths = util::find_paths(meter_id, contribution->value.proxies, num_meters, 0);
    logger->trace("Client {} picked these proxy paths: {}", meter_id, proxy_paths);
    for(const auto& proxy_path : proxy_paths) {
        // Create an encrypted onion for this path and send it
        outgoing_messages.emplace_back(messaging::build_encrypted_onion(proxy_path,
                                                                        contribution, contribution->value.query_num, crypto));
    }
    // Start the overlay by ending "round -1", which will send the messages at the start of round 0
    end_overlay_round();
}

template <typename RecordType>
void ProtocolState<RecordType>::end_overlay_round() {
    // Determine if the Shuffle phase has ended
    if(protocol_phase == ProtocolPhase::SHUFFLE &&
       overlay_round >= 2 * FAILURES_TOLERATED + log2n * log2n + 1) {
        logger->debug("Meter {} is finished with Shuffle", meter_id);
        // Sign each received value and multicast it to the other proxies
        for(const auto& proxy_value : proxy_values) {
            // Create a SignedValue object to hold this value, and add this node's signature to it
            auto signed_value = std::make_shared<messaging::SignedValue<RecordType>>();
            signed_value->value = proxy_value;
            signed_value->signatures[meter_id].fill(0);
            crypto.rsa_sign(*proxy_value, signed_value->signatures[meter_id]);

            std::vector<int> other_proxies(proxy_value->value.proxies.size() - 1);
            std::remove_copy(proxy_value->value.proxies.begin(),
                             proxy_value->value.proxies.end(), other_proxies.begin(), meter_id);
            // Find paths that start at the next round - we send before receive, so we've already sent messages for the current round
            auto proxy_paths = util::find_paths(meter_id, other_proxies, num_meters, overlay_round + 1);
            for(const auto& proxy_path : proxy_paths) {
                // Encrypt with the destination's public key, but don't make an onion
                auto path_message = std::make_shared<messaging::PathOverlayMessage>(
                    get_current_query_num(), proxy_path, signed_value);
                crypto.rsa_encrypt(*path_message, proxy_path.back());
                outgoing_messages.emplace_back(path_message);
            }
        }
        agreement_start_round = overlay_round;
        protocol_phase = ProtocolPhase::AGREEMENT;
    }
    // Detect finishing phase 2 of Agreement
    else if(protocol_phase == ProtocolPhase::AGREEMENT &&
            overlay_round >= agreement_start_round + 4 * FAILURES_TOLERATED + 2 * log2n * log2n + 2 &&
            agreement_phase_state->is_phase1_finished()) {
        logger->debug("Meter {} finished phase 2 of Agreement", meter_id);
        accepted_proxy_values = agreement_phase_state->finish_phase_2();

        // Start the Aggregate phase
        protocol_phase = ProtocolPhase::AGGREGATE;
        start_aggregate_phase();
    }
    // Detect finishing phase 1 of Agreement
    else if(protocol_phase == ProtocolPhase::AGREEMENT &&
            overlay_round >= agreement_start_round + 2 * FAILURES_TOLERATED + log2n * log2n + 1 &&
            !agreement_phase_state->is_phase1_finished()) {
        logger->debug("Meter {} finished phase 1 of Agreement", meter_id);

        auto accept_messages = agreement_phase_state->finish_phase_1(overlay_round);
        outgoing_messages.insert(outgoing_messages.end(), accept_messages.begin(), accept_messages.end());
    }

    // Call the "superclass" end_overlay_round()
    common_end_overlay_round();
}

template <typename RecordType>
void ProtocolState<RecordType>::common_end_overlay_round() {
    timers->cancel_timer(round_timeout_timer);
    // If the last round is ending, the only thing we need to do is cancel the timeout
    if(is_last_round)
        return;

    overlay_round++;
    ping_response_from_predecessor = false;
    // Send outgoing messages at the start of the next round
    send_overlay_message_batch();

    round_timeout_timer = timers->register_timer(OVERLAY_ROUND_TIMEOUT, [this]() { handle_round_timeout(); });

    const int predecessor = util::gossip_predecessor(meter_id, overlay_round, num_meters);
    if(failed_meter_ids.find(predecessor) == failed_meter_ids.end()) {
        // Send a ping to the predecessor meter to see if it's still alive
        auto ping = std::make_shared<messaging::PingMessage>(meter_id, false);
        // This turns out to be really important: Checking whether this ping succeeds
        // is the most common way of detecting that a node has failed
        auto success = network.send(ping, predecessor);
        if(!success) {
            logger->debug("Meter {} detected that meter {} is down", meter_id, predecessor);
            failed_meter_ids.emplace(predecessor);
        }
    }

    // Check future messages in case messages for the next round have already been received
    ptr_list<messaging::OverlayTransportMessage> received_messages;
    for(auto message_iter = future_overlay_messages.begin();
        message_iter != future_overlay_messages.end();) {
        if((*message_iter)->sender_round == overlay_round &&
           std::static_pointer_cast<messaging::OverlayTransportMessage::body_type>((*message_iter)->body)
                   ->query_num == get_current_query_num()) {
            received_messages.emplace_back(*message_iter);
            message_iter = future_overlay_messages.erase(message_iter);
        } else {
            ++message_iter;
        }
    }

    // Cache the last known value of overlay_round, because end_overlay_round()
    // might be called from inside one of these handle()s, so we might already
    // be another round ahead when they return
    const int local_overlay_round = overlay_round;
    for(const auto& message : received_messages) {
        handle_overlay_message(*message);
    }
    // If end_overlay_round() hasn't already been called for another reason,
    // and the predecessor is known to be dead, immediately end the current round
    if(local_overlay_round == overlay_round && failed_meter_ids.find(predecessor) != failed_meter_ids.end()) {
        logger->trace("Meter {} ending round early, predecessor {} is dead", meter_id, predecessor);
        end_overlay_round();
    }
}

template <typename RecordType>
void ProtocolState<RecordType>::send_overlay_message_batch() {
    const int comm_target = util::gossip_target(meter_id, overlay_round, num_meters);
    ptr_list<messaging::OverlayTransportMessage> messages_to_send;
    // First, check waiting messages to see if some are now in the right round
    for(auto message_iter = waiting_messages.begin();
        message_iter != waiting_messages.end();) {
        if((*message_iter)->destination == comm_target) {
            // wrap it up in a new OverlayTransportMessage, then delete from waiting_messages
            messages_to_send.emplace_back(std::make_shared<messaging::OverlayTransportMessage>(
                meter_id, overlay_round, false, *message_iter));
            message_iter = waiting_messages.erase(message_iter);
        } else {
            ++message_iter;
        }
    }
    // Next, check messages generated by the protocol this round to see if they should be sent or held
    for(const auto& overlay_message : outgoing_messages) {
        if(overlay_message->flood || overlay_message->destination == comm_target) {
            messages_to_send.emplace_back(std::make_shared<messaging::OverlayTransportMessage>(
                meter_id, overlay_round, false, overlay_message));
        } else {
            waiting_messages.emplace_back(overlay_message);
        }
    }
    outgoing_messages.clear();
    // Now, send all the messages, marking the last one as final
    if(!messages_to_send.empty()) {
        messages_to_send.back()->is_final_message = true;
        auto success = network.send(messages_to_send, comm_target);
        if(!success) {
            logger->debug("Meter {} detected that meter {} is down", meter_id, comm_target);
            failed_meter_ids.emplace(comm_target);
        }
    } else {
        // If we didn't send anything this round, send an empty message to ensure the target can advance his round
        auto dummy_message = std::make_shared<messaging::OverlayMessage>(
            get_current_query_num(), comm_target, nullptr);
        auto dummy_transport = std::make_shared<messaging::OverlayTransportMessage>(
            meter_id, overlay_round, true, dummy_message);
        logger->trace("Meter {} sending a dummy message to meter {}", meter_id, comm_target);
        auto success = network.send({dummy_transport}, comm_target);
        if(!success) {
            logger->debug("Meter {} detected that meter {} is down", meter_id, comm_target);
            failed_meter_ids.emplace(comm_target);
        }
    }
}

template <typename RecordType>
void ProtocolState<RecordType>::handle_round_timeout() {
    if(ping_response_from_predecessor) {
        ping_response_from_predecessor = false;
        const int predecessor = util::gossip_predecessor(meter_id, overlay_round, num_meters);
        logger->trace("Meter {} continuing to wait for round {}, got a response from {} recently", meter_id, overlay_round, predecessor);
        round_timeout_timer = timers->register_timer(OVERLAY_ROUND_TIMEOUT, [this]() { handle_round_timeout(); });
        auto ping = std::make_shared<messaging::PingMessage>(meter_id, false);
        auto success = network.send(ping, predecessor);
        if(!success) {
            logger->debug("Meter {} detected that meter {} just went down after responding to a ping", meter_id, predecessor);
            failed_meter_ids.emplace(predecessor);
        }
    } else {
        logger->debug("Meter {} timed out waiting for an overlay message for round {}", meter_id, overlay_round);
        end_overlay_round();
    }
}

template <typename RecordType>
void ProtocolState<RecordType>::buffer_future_message(std::shared_ptr<messaging::OverlayTransportMessage> message) {
    future_overlay_messages.push_back(std::move(message));
}
template <typename RecordType>
void ProtocolState<RecordType>::buffer_future_message(std::shared_ptr<messaging::AggregationMessage<RecordType>> message) {
    future_aggregation_messages.push_back(std::move(message));
}

}  // namespace adq