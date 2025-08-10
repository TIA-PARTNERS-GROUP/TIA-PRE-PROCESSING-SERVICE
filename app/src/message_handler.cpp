#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>
#include <string_view>

#include "../include/message_handler.hpp"

// --- Helper Functions ---

/// <summary>
/// Safely retrieves a string from a JSON object. If the key doesn't exist, is
/// null, or the value is a number, it handles the conversion or returns a
/// default value.
/// </summary>
/// <param name="j">The constant reference to the JSON object to query.</param>
/// <param name="key">The key of the value to retrieve.</param>
/// <param name="def">The default string to return if the key is not found or
/// the value is null. Defaults to an empty string.</param> <returns>The
/// retrieved string, a string representation of a number, or the default
/// value.</returns>
std::string get_string_or_default(const json &j, const char *key,
                                  const std::string &def = "") {
  if (j.contains(key) && !j[key].is_null()) {
    // Check if it's a number and convert to string if so
    if (j[key].is_number()) {
      return std::to_string(j[key].get<double>());
    }
    return j[key].get<std::string>();
  }
  return def;
}

/// <summary>
/// Converts a snake_case string to PascalCase for use as a graph node label.
/// It capitalizes the first letter, removes underscores, and capitalizes the
/// letter following an underscore. It also performs basic singularization
/// (e.g., "users" becomes "User", "countries" becomes "Country").
/// </summary>
/// <param name="s">The input string in snake_case.</param>
/// <returns>A string converted to PascalCase.</returns>
std::string to_pascal_case(std::string s) {
  if (s.empty())
    return "";

  // Handle pluralization (C++17 compatible)
  if (s.length() >= 3 && s.substr(s.length() - 3) == "ies") {
    s.pop_back();
    s.pop_back();
    s.pop_back();
    s += "y";
  } else if (!s.empty() && s.back() == 's') {
    s.pop_back();
  }

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

/// <summary>
/// Creates, updates, or deletes a node in Memgraph.
/// For create/update ('c'/'u'), it uses MERGE to create the node if it doesn't
/// exist and sets/updates its properties. For delete ('d'), it finds the node
/// by its ID and performs a DETACH DELETE.
/// </summary>
/// <param name="data">The JSON object containing the node's properties,
/// including a unique 'id'.</param> <param name="op">The character representing
/// the operation: 'c' (create), 'u' (update), or 'd' (delete).</param> <param
/// name="label">The label to be used for the node in Memgraph.</param> <param
/// name="client">A reference to the active MemgraphClient for query
/// execution.</param>
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

/// <summary>
/// Creates or deletes a relationship between two existing nodes in Memgraph.
/// It uses foreign key columns from the source data to identify the start and
/// end nodes.
/// </summary>
/// <param name="data">The JSON object containing the foreign keys for the
/// relationship.</param> <param name="op">The character representing the
/// operation: 'c'/'u' (create) or 'd' (delete).</param> <param
/// name="from_label">The label of the source node for the relationship.</param>
/// <param name="to_label">The label of the target node for the
/// relationship.</param> <param name="rel_type">The type of the relationship
/// (e.g., "HAS_SKILL").</param> <param name="from_fk_col">The column name in
/// 'data' that holds the ID of the source node.</param> <param
/// name="to_fk_col">The column name in 'data' that holds the ID of the target
/// node.</param> <param name="client">A reference to the active MemgraphClient
/// for query execution.</param>
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
    // This section acts as a router, directing the data from a specific table
    // to the correct sequence of node and relationship mapping functions.

    // -- Entity Tables that ALSO define relationships (One-to-Many) --
    if (table == "projects") {
      map_node(data, op, node_label, memgraphClient);
      if (op != 'd' && data.contains("managed_by_user_id") &&
          !data["managed_by_user_id"].is_null()) {
        map_relationship(data, op, "User", "Project", "MANAGES",
                         "managed_by_user_id", "id", memgraphClient);
      }
    } else if (table == "businesses") {
      map_node(data, op, node_label, memgraphClient);
      if (op != 'd') {
        if (data.contains("operator_user_id") &&
            !data["operator_user_id"].is_null())
          map_relationship(data, op, "User", "Business", "OPERATES",
                           "operator_user_id", "id", memgraphClient);
        if (data.contains("business_type_id") &&
            !data["business_type_id"].is_null())
          map_relationship(data, op, "Business", "BusinessType", "IS_TYPE",
                           "id", "business_type_id", memgraphClient);
        if (data.contains("business_category_id") &&
            !data["business_category_id"].is_null())
          map_relationship(data, op, "Business", "BusinessCategory",
                           "IN_CATEGORY", "id", "business_category_id",
                           memgraphClient);
      }
    } else if (table == "skills") {
      map_node(data, op, node_label, memgraphClient);
      if (op != 'd' && data.contains("category_id") &&
          !data["category_id"].is_null()) {
        map_relationship(data, op, "Skill", "SkillCategory", "IN_CATEGORY",
                         "id", "category_id", memgraphClient);
      }
    } else if (table == "strengths") {
      map_node(data, op, node_label, memgraphClient);
      if (op != 'd' && data.contains("category_id") &&
          !data["category_id"].is_null()) {
        map_relationship(data, op, "Strength", "StrengthCategory",
                         "IN_CATEGORY", "id", "category_id", memgraphClient);
      }
    } else if (table == "industries") {
      map_node(data, op, node_label, memgraphClient);
      if (op != 'd' && data.contains("category_id") &&
          !data["category_id"].is_null()) {
        map_relationship(data, op, "Industry", "IndustryCategory",
                         "IN_CATEGORY", "id", "category_id", memgraphClient);
      }
    } else if (table == "ideas") {
      map_node(data, op, node_label, memgraphClient);
      if (op != 'd' && data.contains("submitted_by_user_id") &&
          !data["submitted_by_user_id"].is_null()) {
        map_relationship(data, op, "User", "Idea", "SUBMITTED",
                         "submitted_by_user_id", "id", memgraphClient);
      }
    }
    // -- Simple Entity Tables (become nodes) --
    else if (table == "users" || table == "regions" ||
             table == "subscriptions" || table == "skill_categories" ||
             table == "strength_categories" || table == "business_categories" ||
             table == "business_types" || table == "business_phases" ||
             table == "business_roles" || table == "connection_types" ||
             table == "mastermind_roles" || table == "daily_activities" ||
             table == "case_studies" || table == "industry_categories" ||
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
