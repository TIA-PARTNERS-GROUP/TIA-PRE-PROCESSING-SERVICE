#ifndef MESSAGE_HANDLER_H
#define MESSAGE_HANDLER_H

#include "../external/json.hpp" // Adjust include path as needed
#include "../include/memgraph_client.hpp"
#include <librdkafka/rdkafkacpp.h>

using json = nlohmann::json;

class MessageHandler {
public:
  void Process(RdKafka::Message *msg, MemgraphClient &memgraphClient);
};

#endif // MESSAGE_HANDLER_H
