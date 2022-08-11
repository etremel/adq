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
void TreeAggregationState<RecordType>::initialize(const std::set<int>& failed_meter_ids) {
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
void TreeAggregationState<RecordType>::handle_message(const messaging::AggregationMessage<RecordType>& message, DataSource<RecordType>& data_source) {
    if(aggregation_intermediate->num_contributors == 0) {
        // We have not yet received any values, so no aggregation is necessary; just store the incoming value
        aggregation_intermediate->get_body()->value = message.get_body()->value;
        aggregation_intermediate->num_contributors = message.num_contributors;
    } else {
        // Use the DataSource's aggregation function to combine the incoming message's value with the current intermediate value
        RecordType combined_value = data_source.aggregate_functions.at(current_query->aggregate_function_opcode)(
            std::vector<RecordType>{message.get_body()->value, aggregation_intermediate->get_body()->value},
            current_query->aggregate_serialized_args.data());
        // Update the intermediate value
        aggregation_intermediate->get_body()->value = std::move(combined_value);
        aggregation_intermediate->num_contributors += message.num_contributors;
    }
    children_received_from++;
}

template <typename RecordType>
void TreeAggregationState<RecordType>::compute_and_send_aggregate(
    const util::unordered_ptr_set<messaging::ValueContribution<RecordType>>& accepted_proxy_values,
    DataSource<RecordType>& data_source) {
    // Use the DataSource's aggregation function to combine all the accepted values with the intermediate value, if any
    std::vector<RecordType> values_to_aggregate;
    for(const auto& proxy_value : accepted_proxy_values) {
        values_to_aggregate.emplace_back(proxy_value->value.value);
    }
    if(aggregation_intermediate->num_contributors > 0) {
        values_to_aggregate.emplace_back(aggregation_intermediate->get_body()->value);
    }
    RecordType combined_value = data_source.aggregate_functions.at(current_query->aggregate_function_opcode)(
        values_to_aggregate, current_query->aggregate_serialized_args.data());
    aggregation_intermediate->get_body()->value = std::move(combined_value);
    aggregation_intermediate->num_contributors += accepted_proxy_values.size();

    int parent = util::aggregation_tree_parent(node_id, num_groups, num_meters);
    network.send(aggregation_intermediate, parent);
}

}  // namespace adq