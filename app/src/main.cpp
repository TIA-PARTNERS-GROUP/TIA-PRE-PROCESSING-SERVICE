#include <csignal>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

// 3rd-party libraries
#include "../external/json.hpp"
#include <librdkafka/rdkafkacpp.h>
#include <mgclient.h>

// Use the nlohmann/json library
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

mg_session *initialize_memgraph_session() {
  mg_session_params *params = mg_session_params_make();
  mg_session_params_set_host(params, "memgraph");
  mg_session_params_set_port(params, 7687);

  mg_session *session;
  if (mg_connect(params, &session) != 0) {
    mg_session_params_destroy(params);
    throw std::runtime_error("Failed to connect to Memgraph.");
  }

  mg_session_params_destroy(params);
  std::cout << "✅ Connection to Memgraph successful!" << std::endl;
  return session;
}

// --- Message Processing Function ---

void process_message(RdKafka::Message *msg, mg_session *memgraph_session) {
  if (msg->len() == 0) {
    std::cout
        << "\n[INFO] Received empty message (likely delete event). Skipping."
        << std::endl;
    return;
  }

  const char *payload_str = static_cast<const char *>(msg->payload());

  try {
    auto dbz_event = json::parse(payload_str);

    if (dbz_event.contains("payload") && !dbz_event["payload"].is_null() &&
        dbz_event["payload"].contains("after") &&
        !dbz_event["payload"]["after"].is_null()) {

      auto &user_data = dbz_event["payload"]["after"];
      int user_id = user_data["id"].get<int>();

      // --- FIX: Check for null before getting string values ---
      std::string first_name = user_data["first_name"].is_null()
                                   ? ""
                                   : user_data["first_name"].get<std::string>();
      std::string last_name = user_data["last_name"].is_null()
                                  ? ""
                                  : user_data["last_name"].get<std::string>();

      // --- ADDED: Print extracted user data ---
      std::cout << "\n[DATA] Preparing to sync user data:" << std::endl;
      std::cout << "  - ID: " << user_id << std::endl;
      std::cout << "  - First Name: " << first_name << std::endl;
      std::cout << "  - Last Name: " << last_name << std::endl;
      std::cout << "[QUERY] Executing Cypher..." << std::endl;

      // Use parameterized queries for safety
      mg_map *params = mg_map_make_empty(3);
      mg_map_insert(params, "id", mg_value_make_integer(user_id));
      mg_map_insert(params, "firstName",
                    mg_value_make_string(first_name.c_str()));
      mg_map_insert(params, "lastName",
                    mg_value_make_string(last_name.c_str()));

      const char *query = "MERGE (u:User {id: $id}) SET u.firstName = "
                          "$firstName, u.lastName = $lastName";

      if (mg_session_run(memgraph_session, query, params, NULL, NULL, NULL) !=
          0) {
        std::cerr << "[ERROR] mg_session_run failed for user " << user_id
                  << ": " << mg_session_error(memgraph_session) << std::endl;
      } else if (mg_session_pull(memgraph_session, NULL) < 0) {
        std::cerr << "[ERROR] mg_session_pull failed for user " << user_id
                  << ": " << mg_session_error(memgraph_session) << std::endl;
      } else {
        std::cout << "[SUCCESS] Processed user ID: " << user_id << std::endl;
      }
      mg_map_destroy(params);

    } else {
      std::cout << "\n[INFO] Received message that was not a create/update "
                   "event. Skipping."
                << std::endl;
    }
  } catch (const std::exception &e) { // Catch any standard exception
    std::cerr << "\n[ERROR] Failed to process message: " << e.what()
              << std::endl;
  }
}

// --- Main Application Loop ---

int main() {
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  RdKafka::KafkaConsumer *consumer = nullptr;
  mg_session *memgraph_session = nullptr;

  try {
    consumer = initialize_kafka_consumer();
    memgraph_session = initialize_memgraph_session();

    std::cout << "Starting consumer loop..." << std::endl;
    while (!shutdown_requested) {
      RdKafka::Message *msg = consumer->consume(1000);

      switch (msg->err()) {
      case RdKafka::ERR_NO_ERROR:
        process_message(msg, memgraph_session);
        break;
      case RdKafka::ERR__TIMED_OUT:
        std::cout << "\rWaiting for messages..." << std::flush;
        break;
      default:
        std::cerr << "\n[ERROR] Consumer error: " << msg->errstr() << std::endl;
        break;
      }
      delete msg;
    }

  } catch (const std::exception &e) {
    std::cerr << "A critical error occurred: " << e.what() << std::endl;
  }

  // --- Cleanup ---
  std::cout << "\nShutting down gracefully..." << std::endl;
  if (consumer) {
    consumer->close();
    delete consumer;
  }
  if (memgraph_session) {
    mg_session_destroy(memgraph_session);
  }

  return 0;
}
