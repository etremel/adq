#pragma once

#include "../QueryServer.hpp"

#include "adq/core/CryptoLibrary.hpp"
#include "adq/core/NetworkManager.hpp"
#include "adq/core/ProtocolState.hpp"
#include "adq/messaging/AggregationMessage.hpp"
#include "adq/messaging/QueryRequest.hpp"
#include "adq/messaging/SignatureRequest.hpp"
#include "adq/messaging/SignatureResponse.hpp"
#include "adq/util/LinuxTimerManager.hpp"

#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>

#include <cmath>

namespace adq {

template <typename RecordType>
QueryServer<RecordType>::QueryServer(int num_clients,
                                     uint16_t service_port,
                                     const std::map<int, asio::ip::tcp::endpoint>& client_id_to_ip_map,
                                     const std::string& private_key_filename,
                                     const std::map<int, std::string>& public_key_files_by_id)
    : logger(spdlog::get("global_logger")),
      num_meters(num_clients),
      network(this, service_port, client_id_to_ip_map),
      crypto_library(private_key_filename, public_key_files_by_id),
      timer_library(std::make_unique<util::LinuxTimerManager>()),
      query_timeout_time(compute_timeout_time(num_clients)) {}

template<typename RecordType>
QueryServer<RecordType>::~QueryServer() {
    shut_down();
}

template <typename RecordType>
void QueryServer<RecordType>::listen_loop() {
    network.run();
}

template <typename RecordType>
void QueryServer<RecordType>::shut_down() {
    network.shutdown();
}

template <typename RecordType>
void QueryServer<RecordType>::start_query(std::shared_ptr<messaging::QueryRequest<RecordType>> query) {
    curr_query_meters_signed.clear();
    query_num = query->query_number;
    curr_query_results.clear();
    logger->info("Starting query {}", query_num);
    query_finished = false;
    for(int meter_id = 0; meter_id < num_meters; ++meter_id) {
        network.send(query, meter_id);
    }
    int log2n = std::ceil(std::log2(num_meters));
    int rounds_for_query = 6 * ProtocolState<RecordType>::FAILURES_TOLERATED +
                           3 * log2n * log2n + 3 +
                           (int)std::ceil(std::log2(num_meters / (double)(2 * ProtocolState<RecordType>::FAILURES_TOLERATED + 1)));
    query_timeout_timer = timer_library->register_timer(rounds_for_query * NETWORK_ROUNDTRIP_TIMEOUT, [this]() {
        logger->debug("Utility timed out waiting for query {} after receiving no messages", query_num);
        end_query();
    });
}

template <typename RecordType>
void QueryServer<RecordType>::start_queries(const std::list<std::shared_ptr<messaging::QueryRequest<RecordType>>>& queries) {
    if(queries.empty())
        return;
    for(const auto& query : queries) pending_batch_queries.push(query);
    std::shared_ptr<messaging::QueryRequest<RecordType>> first_query = pending_batch_queries.top();
    pending_batch_queries.pop();
    start_query(first_query);
}

template <typename RecordType>
void QueryServer<RecordType>::end_query() {
    std::shared_ptr<messaging::AggregationMessageValue<RecordType>> query_result;
    for(const auto& result : curr_query_results) {
        logger->debug("Utility results: {}", curr_query_results);
        // Is this the right way to iterate through a multiset and find out the count of each element?
        if((int)curr_query_results.count(result) >= ProtocolState<RecordType>::FAILURES_TOLERATED + 1) {
            query_result = result->get_body();
            break;
        }
    }
    curr_query_results.clear();
    if((int)all_query_results.size() <= query_num) {
        all_query_results.resize(query_num + 1);
    }
    all_query_results[query_num] = query_result;
    if(query_result == nullptr) {
        logger->error("Query {} failed! No results received by timeout.", query_num);
    } else {
        logger->info("Query {} finished, result was {}", query_num, *query_result);
    }
    query_finished = true;
    for(const auto& callback_pair : query_callbacks) {
        callback_pair.second(query_num, query_result);
    }
    if(!pending_batch_queries.empty()) {
        auto next_query = pending_batch_queries.top();
        pending_batch_queries.pop();
        start_query(next_query);
    }
}

template <typename RecordType>
void QueryServer<RecordType>::handle_message(std::shared_ptr<messaging::SignatureRequest<RecordType>> message) {
    if(curr_query_meters_signed.find(message->sender_id) == curr_query_meters_signed.end()) {
        auto signed_value = crypto_library.rsa_sign_blinded(*message->get_body());
        network.send(std::make_shared<messaging::SignatureResponse<RecordType>>(UTILITY_NODE_ID, signed_value), message->sender_id);
        curr_query_meters_signed.insert(message->sender_id);
    }
}

template <typename RecordType>
void QueryServer<RecordType>::handle_message(std::shared_ptr<messaging::AggregationMessage<RecordType>> message) {
    logger->trace("Utility received an aggregation message: {}", *message);
    curr_query_results.insert(message);
    // Clear the timeout, since we got a message
    timer_library->cancel_timer(query_timeout_timer);
    // Check if this was definitely the last result from the query
    if(!query_finished && ((int)curr_query_results.size() > 2 * ProtocolState<RecordType>::FAILURES_TOLERATED)) {
        end_query();
    }
    // If the query isn't finished, set a new timeout for the next result message
    if(!query_finished) {
        query_timeout_timer = timer_library->register_timer(
            query_timeout_time,
            [this]() {
                logger->debug("Utility timed out waiting for query {} after receiving {} messages", query_num, curr_query_results.size());
                end_query();
            });
    }
}

template <typename RecordType>
int QueryServer<RecordType>::register_query_callback(const QueryCallback& callback) {
    int next_id = query_callbacks.empty() ? 0 : query_callbacks.rbegin()->first + 1;
    query_callbacks.emplace(next_id, callback);
    return next_id;
}

template <typename RecordType>
bool QueryServer<RecordType>::deregister_query_callback(const int callback_id) {
    int num_removed = query_callbacks.erase(callback_id);
    return num_removed == 1;
}

template <typename RecordType>
void QueryServer<RecordType>::handle_message(std::shared_ptr<messaging::OverlayTransportMessage<RecordType>> message) {
    logger->warn("Server received an OverlayTransport message. Ignoring it.");
}

template <typename RecordType>
void QueryServer<RecordType>::handle_message(std::shared_ptr<messaging::PingMessage<RecordType>> message) {
    logger->warn("Server received a ping message. Ignoring it.");
}

template <typename RecordType>
void QueryServer<RecordType>::handle_message(std::shared_ptr<messaging::QueryRequest<RecordType>> message) {
    logger->warn("Server received a QueryRequest message. Ignoring it.");
}

template <typename RecordType>
void QueryServer<RecordType>::handle_message(std::shared_ptr<messaging::SignatureResponse<RecordType>> message) {
    logger->warn("Server received a SignatureResponse message. Ignoring it.");
}

template <typename RecordType>
int QueryServer<RecordType>::compute_timeout_time(const int num_meters) {
    int messages_for_aggregation = (int)std::ceil(std::log2((double)num_meters / (double)(2 * ProtocolState<RecordType>::FAILURES_TOLERATED + 1)));
    return messages_for_aggregation * NETWORK_ROUNDTRIP_TIMEOUT;
}

}  // namespace adq