#include <csignal>
#include <iostream>
#include <vector>

#include "../include/kafka_client.hpp"
#include "../include/memgraph_client.hpp"
#include "../include/message_handler.hpp"

// Global flag for graceful shutdown
volatile sig_atomic_t shutdown_requested = 0;

void signal_handler(int sig) { shutdown_requested = 1; }

int main() {
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  // Initialize required libraries
  mg::Client::Init();

  try {
    // 1. Initialize Clients
    KafkaClient kafka("kafka:9092", "memgraph-sync-service");
    MemgraphClient memgraph("memgraph", 7687);
    MessageHandler handler;

    // 2. Subscribe to ALL Kafka Topics (UPDATED AND COMPLETE)
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

    // 3. Run a quick test to ensure Memgraph is working
    memgraph.RunTestQuery();

    std::cout << "\nStarting consumer loop... (Press Ctrl+C to exit)\n"
              << std::endl;

    // 4. Main Application Loop
    while (!shutdown_requested) {
      std::unique_ptr<RdKafka::Message> msg(kafka.Consume(1000));

      switch (msg->err()) {
      case RdKafka::ERR_NO_ERROR:
        try {
          handler.Process(msg.get(), memgraph);
        } catch (const std::runtime_error &e) {
          std::cerr << "\n[ERROR] Could not process message: " << e.what()
                    << std::endl;
        }
        break;
      case RdKafka::ERR__TIMED_OUT:
        // This is expected when no new messages are available
        break;
      default:
        // Handle other Kafka consumer errors
        std::cerr << "\n[WARNING] Consumer error: " << msg->errstr()
                  << std::endl;
        break;
      }
    }

  } catch (const std::exception &e) {
    std::cerr << "A critical error occurred during setup: " << e.what()
              << std::endl;
    mg::Client::Finalize();
    return 1;
  }

  // 5. Cleanup
  std::cout << "\nShutting down gracefully..." << std::endl;
  mg::Client::Finalize();

  return 0;
}
