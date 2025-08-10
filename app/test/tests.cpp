#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <string>

#include "../external/doctest/doctest.h"
#include "../include/message_handler.hpp"

// --- Tests for Helper Functions ---

TEST_CASE("to_pascal_case correctly converts snake_case strings") {
  SUBCASE("Regular plural 's' is removed") {
    CHECK(to_pascal_case("users") == "User");
    CHECK(to_pascal_case("projects") == "Project");
  }

  SUBCASE("Plural 'ies' is converted to 'y'") {
    CHECK(to_pascal_case("categories") == "Category");
    CHECK(to_pascal_case("industries") == "Industry");
  }

  SUBCASE("Multi-word snake_case is converted") {
    CHECK(to_pascal_case("user_skills") == "UserSkill");
    CHECK(to_pascal_case("project_business_categories") ==
          "ProjectBusinessCategory");
  }

  SUBCASE("String with no plural is handled") {
    CHECK(to_pascal_case("data") == "Data");
  }

  SUBCASE("Empty string returns empty string") {
    CHECK(to_pascal_case("") == "");
  }
}

// --- Mock Classes for Testing MessageHandler ---

// A mock MemgraphClient that doesn't connect to a real database.
// It just records the queries that would have been sent.
class MockMemgraphClient : public MemgraphClient {
public:
  // We must override the constructor to prevent it from trying to connect.
  MockMemgraphClient() : MemgraphClient("mock", 0, true) {}

  // Override the ExecuteQuery method to capture the query and params.
  void ExecuteQuery(const std::string &query, const mg::Map &params) override {
    last_query = query;
    // In a real test, you would inspect the params map as well.
  }

  std::string last_query;
};

// --- Tests for MessageHandler::Process ---

TEST_CASE("MessageHandler correctly processes Debezium messages") {
  MessageHandler handler;
  MockMemgraphClient mock_client;

  SUBCASE("Processes a simple 'users' create message") {
    // 1. Create a fake Kafka message with a Debezium JSON payload.
    std::string user_payload = R"({
            "payload": {
                "op": "c",
                "after": { "id": 101, "first_name": "John", "last_name": "Doe" },
                "source": { "table": "users" }
            }
        })";
    // NOTE: In a real test, you'd need to construct a mock RdKafka::Message.
    // This is a conceptual example of the data that would be processed.

    // For this example, we'll manually call a conceptual 'handle_data' method
    // since mocking the full RdKafka::Message is complex.

    // Let's test the logic by creating a query manually and comparing.
    const json data = json::parse(user_payload)["payload"]["after"];
    map_node(data, 'c', "User", mock_client);

    // 2. Check if the correct Cypher query was generated.
    std::string expected_query = "MERGE (n:User {id: $id}) SET n += $props";
    CHECK(mock_client.last_query == expected_query);
  }

  SUBCASE("Processes a 'user_skills' relationship create message") {
    const json data = {{"user_id", 101}, {"skill_id", 202}};
    map_relationship(data, 'c', "User", "Skill", "HAS_SKILL", "user_id",
                     "skill_id", mock_client);

    std::string expected_query = "MATCH (a:User {id: $from_id}) MATCH (b:Skill "
                                 "{id: $to_id}) MERGE (a)-[:HAS_SKILL]->(b)";
    CHECK(mock_client.last_query == expected_query);
  }
}
