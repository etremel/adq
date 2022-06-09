#pragma once

#include "adq/core/InternalTypes.hpp"

#include <spdlog/spdlog.h>
#include <asio.hpp>
#include <map>
#include <memory>

namespace adq {

template <typename RecordType>
class NetworkManager {
private:
    std::shared_ptr<spdlog::logger> logger;
    /** The io_context that all the sockets will use */
    asio::io_context network_io_context;
    /** The QueryClient that owns this NetworkManager */
    QueryClient<RecordType>& query_client;
    /** Maps client IDs to address/port pairs. */
    std::map<int, asio::ip::tcp::endpoint> id_to_ip_map;
    /** Maps address/port pairs to client IDs */
    std::map<asio::ip::tcp::endpoint, int> ip_to_id_map;
    /**
     * Cache of open sockets to clients, lazily initialized: the socket
     * is created the first time a message is sent to or received from
     * that client. This may also contain a socket for the query server,
     * at entry -1.
     */
    std::map<int, asio::ip::tcp::socket> sockets_by_id;
    /**
     * Contains one receive buffer per client for reading the initial
     * "size of message" data for an incoming message. These will be
     * supplied to asynchronous read requests.
     */
    std::map<int, std::array<uint8_t, sizeof(std::size_t)>> length_buffers;
    /**
     * Contains one receive buffer per client that will be used to read a
     * (complete) message from a socket. These will be allocated dynamically
     * once the size of an incoming message is known.
     */
    std::map<int, std::vector<uint8_t>> incoming_message_buffers;

    /**
     * A "server socket" that listens for incoming connections from
     * other clients
     */
    asio::ip::tcp::acceptor connection_listener;

    /**
     * Handler function for ASIO accept events.
     */
    void handle_accept(const asio::error_code& error, asio::ip::tcp::socket incoming_socket);

    /**
     * Handles an asynchronous read event for the "size of message" header
     * from a single client. When ASIO calls this function, the message size
     * should be in the length_buffers entry for the specified client.
     */
    void handle_size_read(int sender_id, const asio::error_code& error, std::size_t bytes_read);

    /**
     * Handles an asynchronous read event for the body of a message from a client.
     * When ASIO calls this function, the incoming message buffer for that client
     * should be populated with data.
     */
    void handle_body_read(int sender_id, const asio::error_code& error, std::size_t bytes_read);

    /**
     * Starts an asynchronous accept request on the connection listener.
     */
    void do_accept();

    /**
     * Starts an asynchronous read on the socket for the specified client ID
     * that expects to read exactly sizeof(size_t) bytes. This is the standard
     * "header" of a message indicating the length of its body.
     *
     * @param sender_id The ID of the client to read from
     */
    void start_size_read(int sender_id);

    /**
     * Starts an asynchronous read on the socket for the specified client ID
     * that expects to read the specified number of bytes. This will allocate
     * a buffer of the specified size in incoming_message_buffers.
     *
     * @param sender_id The ID of the client to read from
     * @param body_size The number of bytes to read
     */
    void start_body_read(int sender_id, std::size_t body_size);

    /**
     * Performs the application-level logic of reading a message and dispatching
     * it to the correct handler, assuming the entire message has already been
     * received into a buffer.
     *
     * @param message_bytes A reference to the byte buffer containing the body of the message
     */
    void receive_message(const std::vector<uint8_t>& message_bytes);

    /**
     * Handles an asynchronous write event for one of the "send" functions. Since
     * there's nothing left to do once a write completes, this just does error
     * reporting.
     *
     * @param recipient_id The ID of the client that the send was writing to
     * @param error An ASIO error code, if any
     * @param bytes_sent The number of bytes successfully written
     */
    void handle_write_complete(int recipient_id, const asio::error_code& error, std::size_t bytes_sent);

public:
    /**
     * Constructs a NetworkManager that is owned by a QueryClient object and
     * listens for connections on a particular port.
     *
     * @param owning_client A reference to the QueryClient object that created
     * this NetworkManager; used to deliver received messages back to the QueryClient
     * @param service_port The port that the NetworkManager should listen on for
     * incoming connections.
     * @param client_id_to_ip_map The QueryClient's map from client IDs to IP addresses
     */
    NetworkManager(QueryClient<RecordType>& owning_client, uint16_t service_port,
                   const std::map<int, asio::ip::tcp::endpoint>& client_id_to_ip_map);

    /**
     * Starts waiting for network events by giving control of the calling
     * thread to the ASIO io_context. Callers should expect this function
     * to block forever.
     */
    void run();

    /**
     * Sends a stream of overlay messages over the network to another meter,
     * identified by its ID. Messages will be sent in the order they appear
     * in the list.
     * @param messages The messages to send.
     * @param recipient_id The ID of the recipient.
     * @return true if the send was successful, false if a connection could not be made.
     */
    bool send(const std::list<std::shared_ptr<messaging::OverlayTransportMessage>>& messages, const int recipient_id);
    /**
     * Sends an AggregationMessage over the network to another meter (or the
     * utility), identified by its ID.
     * @param message The message to send
     * @param recipient_id The ID of the recipient
     * @return true if the send was successful, false if a connection could not be made.
     */
    bool send(const std::shared_ptr<messaging::AggregationMessage>& message, const int recipient_id);
    /**
     * Sends a PingMessage over the network to another meter
     * @param message The message to send
     * @param recipient_id The ID of the recipient
     * @return true if the send was successful, false if a connection could not be made.
     */
    bool send(const std::shared_ptr<messaging::PingMessage>& message, const int recipient_id);
    /** Sends a signature request message to the query server. */
    bool send(const std::shared_ptr<messaging::SignatureRequest>& message);
};
}  // namespace adq

#include "detail/NetworkManager_impl.hpp"