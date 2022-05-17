#include "../QueryClient.hpp"
#include "adq/core/DataSource.hpp"

namespace adq {

template<typename RecordType>
void QueryClient<RecordType>::handle_message(const std::shared_ptr<messaging::QueryRequest>& message) {
    //Forward the serialized function call to the DataSource object
    RecordType data_to_contribute = data_source->select_functions.at(message->select_function_opcode)(message->select_serialized_args);
    bool should_contribute = data_source->filter_functions.at(message->filter_function_opcode)(data_to_contribute, message->filter_serialized_args);
    if(should_contribute) {
        query_protocol_state.start_query(message, data_to_contribute);
    }
}

}