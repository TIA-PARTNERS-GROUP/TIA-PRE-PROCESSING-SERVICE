#ifndef MESSAGE_HANDLER_H
#define MESSAGE_HANDLER_H

#include "../external/json.hpp" // Adjust include path as needed
#include "../include/memgraph_client.hpp"
#include <librdkafka/rdkafkacpp.h>

// Alias for the nlohmann::json class for convenience.
using json = nlohmann::json;

/// <summary>
/// Handles the core business logic of the service.
/// Its primary responsibility is to parse incoming Kafka messages, interpret
/// them as database change events, and translate them into corresponding Cypher
/// queries to be executed against a Memgraph database.
/// </summary>
class MessageHandler {
public:
  /// <summary>
  /// Processes a single Kafka message, expected to be a Debezium CDC event in
  /// JSON format. It parses the message, identifies the database operation and
  /// source table, and then routes the data to the appropriate mapping function
  /// to reflect the change in Memgraph.
  /// </summary>
  /// <param name="msg">A pointer to the consumed RdKafka::Message to be
  /// processed.</param> <param name="memgraphClient">A reference to the
  /// MemgraphClient used for all database interactions.</param> <exception
  /// cref="std::runtime_error">Throws if message processing fails, e.g., due to
  /// JSON parsing errors.</exception>
  void Process(RdKafka::Message *msg, MemgraphClient &memgraphClient);
};

#endif // MESSAGE_HANDLER_H
