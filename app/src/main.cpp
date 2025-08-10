#include <csignal>
#include <iostream>
#include <vector>

#include "../include/kafka_client.hpp"
#include "../include/memgraph_client.hpp"
#include "../include/message_handler.hpp"

/// <summary>
/// A global, thread-safe flag to signal that the application should shut down
/// gracefully. It is of type 'volatile sig_atomic_t' to ensure safe access from
/// a signal handler.
/// </summary>
volatile sig_atomic_t shutdown_requested = 0;

/// <summary>
/// Signal handler for SIGINT (Ctrl+C) and SIGTERM. Sets the global
/// shutdown_requested flag.
/// </summary>
/// <param name="sig">The signal number that was caught.</param>
void signal_handler(int sig) { shutdown_requested = 1; }

/// <summary>
/// The main entry point for the Kafka-to-Memgraph synchronization service.
/// This application connects to a Kafka cluster, subscribes to a set of topics
/// representing database change events (CDC), and processes these messages to
/// update a Memgraph graph database. It handles graceful shutdown via SIGINT
/// and SIGTERM signals.
/// </summary>
/// <returns>0 on successful execution and graceful shutdown, 1 on a critical
/// error.</returns>
int main() {
  // Register signal handlers for graceful shutdown.
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  // Initialize required third-party libraries.
  mg::Client::Init();

  try {
    // 1. Initialize Clients
    // Establish connections to Kafka and Memgraph.
    KafkaClient kafka("kafka:9092", "memgraph-sync-service");
    MemgraphClient memgraph("memgraph", 7687);
    MessageHandler handler;

    // 2. Subscribe to ALL Kafka Topics
    // The topics list corresponds to Debezium topics for tables in a relational
    // database.
    std::vector<std::string> topics = {
        "tia_server.dev_tia_db.business_categories",
        "tia_server.dev_tia_db.business_connections",
        "tia_server.dev_tia_db.business_phases",
        "tia_server.dev_tia_db.business_roles",
        "tia_server.dev_tia_db.business_skills",
        "tia_server.dev_tia_db.business_strengths",
        "tia_server.dev_tia_db.business_types",
        "tia_server.dev_tia_db.businesses",
        "tia_server.dev_tia_db.case_studies",
        "tia_server.dev_tia_db.connection_mastermind_roles",
        "tia_server.dev_tia_db.connection_types",
        "tia_server.dev_tia_db.daily_activities",
        "tia_server.dev_tia_db.daily_activity_enrolments",
        "tia_server.dev_tia_db.idea_votes",
        "tia_server.dev_tia_db.ideas",
        "tia_server.dev_tia_db.industries",
        "tia_server.dev_tia_db.industry_categories",
        "tia_server.dev_tia_db.mastermind_roles",
        "tia_server.dev_tia_db.notifications",
        "tia_server.dev_tia_db.project_business_categories",
        "tia_server.dev_tia_db.project_business_skills",
        "tia_server.dev_tia_db.project_regions",
        "tia_server.dev_tia_db.projects",
        "tia_server.dev_tia_db.regions",
        "tia_server.dev_tia_db.skill_categories",
        "tia_server.dev_tia_db.skills",
        "tia_server.dev_tia_db.strength_categories",
        "tia_server.dev_tia_db.strengths",
        "tia_server.dev_tia_db.subscriptions",
        "tia_server.dev_tia_db.user_business_strengths",
        "tia_server.dev_tia_db.user_daily_activity_progress",
        "tia_server.dev_tia_db.user_logins",
        "tia_server.dev_tia_db.user_posts",
        "tia_server.dev_tia_db.user_skills",
        "tia_server.dev_tia_db.user_strengths",
        "tia_server.dev_tia_db.user_subscriptions",
        "tia_server.dev_tia_db.users"};
    kafka.Subscribe(topics);

    // 3. Run a quick test to ensure Memgraph is working and accessible.
    memgraph.RunTestQuery();

    std::cout << "\nStarting consumer loop... (Press Ctrl+C to exit)\n"
              << std::endl;

    // 4. Main Application Loop
    // Continuously polls Kafka for new messages until a shutdown is requested.
    while (!shutdown_requested) {
      // Consume a message with a 1-second timeout.
      std::unique_ptr<RdKafka::Message> msg(kafka.Consume(1000));

      switch (msg->err()) {
      case RdKafka::ERR_NO_ERROR:
        // A valid message was received.
        try {
          handler.Process(msg.get(), memgraph);
        } catch (const std::runtime_error &e) {
          std::cerr << "\n[ERROR] Could not process message: " << e.what()
                    << std::endl;
        }
        break;
      case RdKafka::ERR__TIMED_OUT:
        // No message received within the timeout. This is normal and expected.
        break;
      default:
        // An actual Kafka consumer error occurred.
        std::cerr << "\n[WARNING] Consumer error: " << msg->errstr()
                  << std::endl;
        break;
      }
    }

  } catch (const std::exception &e) {
    std::cerr << "A critical error occurred during setup: " << e.what()
              << std::endl;
    // Ensure cleanup is still performed on catastrophic failure.
    mg::Client::Finalize();
    return 1;
  }

  // 5. Cleanup
  // Perform a clean shutdown of all client libraries.
  std::cout << "\nShutting down gracefully..." << std::endl;
  mg::Client::Finalize();

  return 0;
}
