#include <csignal>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

// 3rd-party libraries
#include "../external/json.hpp" // Correct include path based on your Dockerfile
#include <librdkafka/rdkafkacpp.h>
#include <mgclient.hpp> // Use the C++ wrapper header

using json = nlohmann::json;

// Global flag for graceful shutdown
volatile sig_atomic_t shutdown_requested = 0;

void signal_handler(int sig) { shutdown_requested = 1; }

// --- Initialization Functions ---

RdKafka::KafkaConsumer *initialize_kafka_consumer() {
  std::string errstr;
  RdKafka::Conf *conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);

  conf->set("bootstrap.servers", "kafka:9092", errstr);
  conf->set("group.id", "memgraph-sync-service", errstr);
  conf->set("auto.offset.reset", "earliest", errstr);

  RdKafka::KafkaConsumer *consumer =
      RdKafka::KafkaConsumer::create(conf, errstr);
  delete conf;

  if (!consumer) {
    throw std::runtime_error("Failed to create Kafka consumer: " + errstr);
  }

  std::vector<std::string> topics{"tia_server.dev_tia_db.users"};
  consumer->subscribe(topics);
  std::cout << "✅ Subscribed to Kafka topic: " << topics[0] << std::endl;

  return consumer;
}

// Reverted to initializing a C++ mg::Client object
std::unique_ptr<mg::Client> initialize_memgraph_client() {
  mg::Client::Params params;
  params.host = "memgraph";
  params.port = 7687;

  auto client = mg::Client::Connect(params);
  if (!client) {
    throw std::runtime_error("Failed to connect to Memgraph.");
  }

  std::cout << "✅ Connection to Memgraph successful!" << std::endl;
  return client;
}

// --- DEBUGGING: Test Function ---
void run_memgraph_test_query(mg::Client *client) {
  std::cout << "\n[TEST] Running a simple test query..." << std::endl;
  if (!client->Execute("CREATE (n:TestNode {property: 'hello world'})")) {
    throw std::runtime_error("Test query failed: ");
  }
  client->DiscardAll();
  std::cout << "[TEST] Test query successful!" << std::endl;
}

// Reverted to using the C++ API
void process_message(RdKafka::Message *msg, mg::Client *client) {
  if (msg->len() == 0) {
    std::cout
        << "\n[INFO] Received empty message (likely delete event). Skipping."
        << std::endl;
    return;
  }

  const char *payload_str = static_cast<const char *>(msg->payload());

  try {
    auto dbz_event = json::parse(std::string_view(payload_str, msg->len()));

    if (dbz_event.contains("payload") && !dbz_event["payload"].is_null() &&
        dbz_event["payload"].contains("after") &&
        !dbz_event["payload"]["after"].is_null()) {

      auto &user_data = dbz_event["payload"]["after"];
      int user_id = user_data["id"].get<int>();

      std::string first_name = user_data["first_name"].is_null()
                                   ? ""
                                   : user_data["first_name"].get<std::string>();
      std::string last_name = user_data["last_name"].is_null()
                                  ? ""
                                  : user_data["last_name"].get<std::string>();

      std::cout << "\n[DATA] Preparing to sync user data:" << std::endl;
      std::cout << "  - ID: " << user_id << std::endl;
      std::cout << "  - First Name: " << first_name << std::endl;
      std::cout << "  - Last Name: " << last_name << std::endl;
      std::cout << "[QUERY] Executing Cypher..." << std::endl;

      mg::Map params(3);
      params.Insert("id", mg::Value(user_id));
      params.Insert("firstName", mg::Value(first_name));
      params.Insert("lastName", mg::Value(last_name));

      const char *query = "MERGE (u:User {id: $id}) SET u.firstName = "
                          "$firstName, u.lastName = $lastName";

      if (!client->Execute(query, params.AsConstMap())) {
        throw std::runtime_error("Failed to execute query for user " +
                                 std::to_string(user_id));
      }

      client->DiscardAll();

      std::cout << "[SUCCESS] Processed user ID: " << user_id << std::endl;
    } else {
      std::cout << "\n[INFO] Received message that was not a create/update "
                   "event. Skipping."
                << std::endl;
    }
  } catch (const json::parse_error &e) {
    throw std::runtime_error(std::string("Failed to parse JSON message: ") +
                             e.what());
  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("Failed to process message: ") +
                             e.what());
  }
}
// --- Main Application Loop ---

int main() {
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  // ADDED: Initialize the mgclient library
  mg::Client::Init();

  RdKafka::KafkaConsumer *consumer = nullptr;
  std::unique_ptr<mg::Client> memgraph_client;

  try {
    consumer = initialize_kafka_consumer();
    memgraph_client = initialize_memgraph_client();

    run_memgraph_test_query(memgraph_client.get());

    std::cout << "\nStarting consumer loop...\n" << std::endl;
    while (!shutdown_requested) {
      RdKafka::Message *msg = consumer->consume(1000);

      try {
        switch (msg->err()) {
        case RdKafka::ERR_NO_ERROR:
          process_message(msg, memgraph_client.get());
          break;
        case RdKafka::ERR__TIMED_OUT:
          break;
        default:
          std::cerr << "\n[WARNING] Consumer error: " << msg->errstr()
                    << std::endl;
          break;
        }
      } catch (const std::runtime_error &e) {
        std::cerr << "\n[ERROR] Could not process message: " << e.what()
                  << std::endl;
      }
      delete msg;
    }

  } catch (const std::exception &e) {
    std::cerr << "A critical error occurred: " << e.what() << std::endl;
    mg::Client::Finalize(); // Finalize even on error
    return 1;
  }

  // --- Cleanup ---
  std::cout << "\nShutting down gracefully..." << std::endl;
  if (consumer) {
    consumer->close();
    delete consumer;
  }

  // ADDED: Finalize the mgclient library
  mg::Client::Finalize();

  return 0;
}
