#include "../include/message_handler.hpp"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>
#include <string_view>

// --- Helper Functions ---

// Safely gets a string from JSON or returns a default value.
std::string get_string_or_default(const json &j, const char *key,
                                  const std::string &def = "") {
  if (j.contains(key) && !j[key].is_null()) {
    return j[key].get<std::string>();
  }
  return def;
}

// Converts a snake_case string to PascalCase for node labels (e.g.,
// "user_skills" -> "UserSkill").
std::string to_pascal_case(std::string s) {
  if (s.empty())
    return "";
  s[0] = std::toupper(s[0]);
  for (size_t i = 1; i < s.length(); ++i) {
    if (s[i - 1] == '_') {
      s[i] = std::toupper(s[i]);
    }
  }
  s.erase(std::remove(s.begin(), s.end(), '_'), s.end());
  return s;
}

// --- Generic Mapping Functions ---

// Generic function to create or delete a node.
void map_node(const json &data, char op, const std::string &label,
              MemgraphClient &client) {
  const std::string query =
      (op == 'd') ? "MATCH (n:" + label + " {id: $id}) DETACH DELETE n"
                  : "MERGE (n:" + label + " {id: $id}) SET n += $props";

  mg::Map params(op == 'd' ? 1 : 2);
  params.Insert("id", mg::Value(data["id"].get<int>()));

  if (op != 'd') {
    mg::Map props(data.size());
    for (auto &[key, value] : data.items()) {
      if (value.is_string())
        props.Insert(key, mg::Value(value.get<std::string>()));
      else if (value.is_number_integer())
        props.Insert(key, mg::Value(value.get<int>()));
      else if (value.is_number_float())
        props.Insert(key, mg::Value(value.get<double>()));
      else if (value.is_boolean())
        props.Insert(key, mg::Value(value.get<bool>()));
    }
    params.Insert("props", mg::Value(std::move(props)));
  }
  client.ExecuteQuery(query, params);
}

// Generic function to create or delete a relationship between two nodes.
void map_relationship(const json &data, char op, const std::string &from_label,
                      const std::string &to_label, const std::string &rel_type,
                      const std::string &from_fk_col,
                      const std::string &to_fk_col, MemgraphClient &client) {
  const std::string query =
      (op == 'd')
          ? "MATCH (a:" + from_label + " {id: $from_id})-[r:" + rel_type +
                "]->(b:" + to_label + " {id: $to_id}) DELETE r"
          : "MATCH (a:" + from_label + " {id: $from_id}) MATCH (b:" + to_label +
                " {id: $to_id}) MERGE (a)-[:" + rel_type + "]->(b)";

  mg::Map params(2);
  params.Insert("from_id", mg::Value(data[from_fk_col].get<int>()));
  params.Insert("to_id", mg::Value(data[to_fk_col].get<int>()));
  client.ExecuteQuery(query, params);
}

// --- Main Processing Logic ---

void MessageHandler::Process(RdKafka::Message *msg,
                             MemgraphClient &memgraphClient) {
  if (msg->len() == 0)
    return;

  try {
    auto dbz_event = json::parse(std::string_view(
        static_cast<const char *>(msg->payload()), msg->len()));

    if (!dbz_event.contains("payload") || dbz_event["payload"].is_null())
      return;

    auto &payload = dbz_event["payload"];
    char op = payload["op"].get<std::string>()[0];
    const auto &data = (op == 'd') ? payload["before"] : payload["after"];
    if (data.is_null())
      return;

    std::string table = payload["source"]["table"].get<std::string>();
    std::string node_label = to_pascal_case(table);

    // --- MAPPING ROUTER ---
    // This router directs each table to the correct mapping logic.

    // -- Simple Entity Tables (become nodes) --
    if (table == "users" || table == "skills" || table == "projects" ||
        table == "businesses" || table == "regions" ||
        table == "subscriptions" || table == "strengths" || table == "ideas" ||
        table == "skill_categories" || table == "strength_categories" ||
        table == "business_categories" || table == "business_types" ||
        table == "business_phases" || table == "business_roles" ||
        table == "connection_types" || table == "mastermind_roles" ||
        table == "daily_activities" || table == "case_studies" ||
        table == "industry_categories" || table == "industries" ||
        table == "notifications" || table == "business_connections" ||
        table == "user_posts") {
      map_node(data, op, node_label, memgraphClient);
    }
    // -- One-to-One Relationships (property merge) --
    else if (table == "user_logins") {
      if (op != 'd') {
        const std::string query =
            "MERGE (u:User {id: $user_id}) SET u.loginEmail = $login_email";
        mg::Map params(2);
        params.Insert("user_id", mg::Value(data["user_id"].get<int>()));
        // CORRECTED: Wrap the string in mg::Value()
        params.Insert("login_email",
                      mg::Value(get_string_or_default(data, "login_email")));
        memgraphClient.ExecuteQuery(query, params);
      }
    }
    // -- Join Tables (become relationships) --
    else if (table == "project_regions") {
      map_relationship(data, op, "Project", "Region", "IN_REGION", "project_id",
                       "region_id", memgraphClient);
    } else if (table == "user_skills") {
      map_relationship(data, op, "User", "Skill", "HAS_SKILL", "user_id",
                       "skill_id", memgraphClient);
    } else if (table == "user_strengths") {
      map_relationship(data, op, "User", "Strength", "HAS_STRENGTH", "user_id",
                       "strength_id", memgraphClient);
    } else if (table == "project_business_skills") {
      map_relationship(data, op, "Project", "BusinessSkill", "REQUIRES_SKILL",
                       "project_id", "business_skill_id", memgraphClient);
    } else if (table == "project_business_categories") {
      map_relationship(data, op, "Project", "BusinessCategory", "IN_CATEGORY",
                       "project_id", "business_category_id", memgraphClient);
    } else if (table == "daily_activity_enrolments") {
      map_relationship(data, op, "User", "DailyActivity", "ENROLLED_IN",
                       "user_id", "daily_activity_id", memgraphClient);
    } else if (table == "idea_votes") {
      map_relationship(data, op, "User", "Idea", "VOTED_FOR", "voter_user_id",
                       "idea_id", memgraphClient);
    } else if (table == "user_subscriptions") {
      map_relationship(data, op, "User", "Subscription", "HAS_SUBSCRIPTION",
                       "user_id", "subscription_id", memgraphClient);
    } else if (table == "user_business_strengths") {
      map_relationship(data, op, "User", "BusinessStrength",
                       "HAS_BUSINESS_STRENGTH", "user_id",
                       "business_strength_id", memgraphClient);
    } else if (table == "user_daily_activity_progress") {
      map_relationship(data, op, "User", "DailyActivity", "HAS_PROGRESS_IN",
                       "user_id", "daily_activity_id", memgraphClient);
    } else if (table == "connection_mastermind_roles") {
      map_relationship(data, op, "BusinessConnection", "MastermindRole",
                       "HAS_MASTERMIND_ROLE", "connection_id",
                       "mastermind_role_id", memgraphClient);
    }

    std::cout << "[SUCCESS] Processed op '" << op << "' for table '" << table
              << "'" << std::endl;

  } catch (const std::exception &e) {
    throw std::runtime_error(
        std::string("Failed to process message for topic: ") +
        msg->topic_name() + " | " + e.what());
  }
}
