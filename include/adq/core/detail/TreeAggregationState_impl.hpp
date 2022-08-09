#pragma once

#include "../TreeAggregationState.hpp"

#include "adq/core/NetworkManager.hpp"
#include "adq/messaging/AggregationMessage.hpp"
#include "adq/util/Overlay.hpp"
#include "adq/util/PointerUtil.hpp"

#include <memory>
#include <set>
#include <tuple>

namespace adq {

template <typename RecordType>
void TreeAggregationState<RecordType>::initialize(const int data_array_length, const std::set<int>& failed_meter_ids) {
    aggregation_intermediate = std::make_shared<messaging::AggregationMessage<RecordType>>(
        node_id, current_query->query_number,
        std::make_shared<messaging::AggregationMessageValue<RecordType>>(data_array_length));
    children_received_from = 0;
    std::pair<int, int> children = util::aggregation_tree_children(node_id, num_groups, num_meters);
    children_needed = 2;
    //                          "failed_meter_ids contains children.first"
    if(children.first == -1 || failed_meter_ids.find(children.first) != failed_meter_ids.end()) {
        children_needed--;
    }
    if(children.second == -1 || failed_meter_ids.find(children.second) != failed_meter_ids.end()) {
        children_needed--;
    }
    initialized = true;
}

template <typename RecordType>
bool TreeAggregationState<RecordType>::done_receiving_from_children() const {
    return children_received_from >= children_needed;
}

template <typename RecordType>
void TreeAggregationState<RecordType>::handle_message(const messaging::AggregationMessage<RecordType>& message) {
    // PROBLEM: There is no "single value" or "vector value" distinction any more; all queries return a single
    // value of type RecordType, but RecordType may itself be a vector (which it is in the SmartMeters example)
    // TODO: Use the AggregateFunction from QueryFunctions to combine values from other clients; this will
    // require asking the QueryClient for its DataSource.
    // TODO: Replace AggregationMessageValue's built-in vector with a single value of type RecordType
    if(is_single_valued_query(current_query->request_type)) {
        aggregation_intermediate->add_value(message.get_body()->at(0), message.get_num_contributors());
    } else {
        aggregation_intermediate->add_values(*message.get_body(), message.get_num_contributors());
    }
    children_received_from++;
}

template <typename RecordType>
void TreeAggregationState<RecordType>::compute_and_send_aggregate(
    const util::unordered_ptr_set<messaging::ValueContribution<RecordType>>& accepted_proxy_values) {
    for(const auto& proxy_value : accepted_proxy_values) {
        // temporarily omitted: Input Sanity Check
        if(is_single_valued_query(current_query->request_type)) {
            aggregation_intermediate->add_value(proxy_value->value.value[0], 1);
        } else {
            aggregation_intermediate->add_values(proxy_value->value.value, 1);
        }
    }
    int parent = util::aggregation_tree_parent(node_id, num_groups, num_meters);
    network.send(aggregation_intermediate, parent);
}

}  // namespace adq