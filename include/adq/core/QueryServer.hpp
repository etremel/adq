
#include "InternalTypes.hpp"
#include "CryptoLibrary.hpp"
#include "NetworkManager.hpp"
#include "adq/util/TimerManager.hpp"

#include <spdlog/spdlog.h>

#include <map>
#include <memory>
#include <queue>
#include <vector>
#include <list>
#include <set>

namespace adq {

template <typename RecordType>
class QueryServer {
public:
    using QueryCallback = std::function<void(const int, std::shared_ptr<messaging::AggregationMessageValue>)>;

private:
    std::shared_ptr<spdlog::logger> logger;
    const int num_meters;
    NetworkManager<RecordType> network;
    CryptoLibrary crypto_library;
    std::unique_ptr<util::TimerManager> timer_library;
    /** Number of milliseconds to wait for a query timeout interval */
    const int query_timeout_time;
    /** Handle referring to the timer that was set to time-out the current query*/
    int query_timeout_timer;
    int query_num;
    bool query_finished;
    std::map<int, QueryCallback> query_callbacks;
    util::unordered_ptr_multiset<messaging::AggregationMessage> curr_query_results;
    /** All results of queries the utility has issued, indexed by query number. */
    std::vector<std::shared_ptr<messaging::AggregationMessageValue>> all_query_results;
    std::set<int> curr_query_meters_signed;
    //"A priority queue of pointers to QueryRequests, ordered by QueryNumGreater"
    using query_priority_queue = std::priority_queue<
        std::shared_ptr<messaging::QueryRequest>,
        std::vector<std::shared_ptr<messaging::QueryRequest>>,
        util::ptr_comparator<messaging::QueryRequest, messaging::QueryNumGreater>>;
    query_priority_queue pending_batch_queries;
    static int compute_timeout_time(const int num_meters);

public:
    /** Handles receiving an AggregationMessage from a meter, which should contain a query result. */
    void handle_message(const std::shared_ptr<messaging::AggregationMessage>& message);

    /** Handles receiving a SignatureRequest from a meter, by signing the requested value. */
    void handle_message(const std::shared_ptr<messaging::SignatureRequest>& message);

    /** Starts a query by broadcasting a message from the utility to all the meters in the network.
     * Do not call this while an existing query is still in progress, or the existing query's
     * results will be lost. */
    void start_query(const std::shared_ptr<messaging::QueryRequest>& query);

    /** Starts a batch of queries that should be executed in sequence as quickly as possible */
    void start_queries(const std::list<std::shared_ptr<messaging::QueryRequest>>& queries);

    /** Registers a callback function that should be run each time a query completes. */
    int register_query_callback(const QueryCallback& callback);

    /** Deregisters a callback function previously registered, using its ID. */
    bool deregister_query_callback(const int callback_id);

    /** Gets the stored result of a query that has completed. */
    std::shared_ptr<messaging::AggregationMessageValue> get_query_result(const int query_num) { return all_query_results.at(query_num); }

    /** Starts an infinite loop that listens for incoming messages (i.e.
     * query responses) and reacts to them. This function call never
     * returns, so it should be run in a separate thread from any start_query
     * calls. */
    void listen_loop();

    /** Shuts down the message-listening loop to allow the client to exit cleanly.
     * Obviously, this must be called from a separate thread from listen_loop().  */
    void shut_down();

    /** The maximum time (ms) the utility is willing to wait on a network round-trip */
    static constexpr int NETWORK_ROUNDTRIP_TIMEOUT = 100;
};
}  // namespace adq