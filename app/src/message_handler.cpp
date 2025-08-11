#include <algorithm>
#include <cctype>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

#include "../include/message_handler.hpp"

// --- Helper Functions (Unchanged) ---
std::string get_string_or_default(const json &j, const char *key,
                                  const std::string &def = "") {
  if (j.contains(key) && !j[key].is_null()) {
    if (j[key].is_number()) {
      return std::to_string(j[key].get<double>());
    }
    return j[key].get<std::string>();
  }
  return def;
}

std::string to_pascal_case(std::string s) {
  if (s.empty())
    return "";

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

// --- Generic Mapping Functions with Caching Logic ---

void map_node(const json &data, char op, const std::string &label,
              MemgraphClient &client,
              std::unordered_map<std::string, std::string> &query_cache) {

  std::string query;
  const std::string cache_key = std::string(1, op) + "_node_" + label;

  auto it = query_cache.find(cache_key);
  if (it != query_cache.end()) {
    query = it->second;
  } else {
    query = (op == 'd') ? "MATCH (n:" + label + " {id: $id}) DETACH DELETE n"
                        : "MERGE (n:" + label + " {id: $id}) SET n += $props";
    query_cache[cache_key] = query;
  }

  mg::Map params(op == 'd' ? 1 : 2);
  if (data["id"].is_string()) {
    params.Insert("id", mg::Value(data["id"].get<std::string>()));
  } else {
    params.Insert("id", mg::Value(data["id"].get<int>()));
  }

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

void map_relationship(
    const json &data, char op, const std::string &from_label,
    const std::string &to_label, const std::string &rel_type,
    const std::string &from_fk_col, const std::string &to_fk_col,
    MemgraphClient &client,
    std::unordered_map<std::string, std::string> &query_cache) {

  std::string query;
  const std::string cache_key = std::string(1, op) + "_rel_" + from_label +
                                "_" + rel_type + "_" + to_label;

  auto it = query_cache.find(cache_key);
  if (it != query_cache.end()) {
    query = it->second;
  } else {
    query = (op == 'd')
                ? "MATCH (a:" + from_label + " {id: $from_id})-[r:" + rel_type +
                      "]->(b:" + to_label + " {id: $to_id}) DELETE r"
                : "MATCH (a:" + from_label +
                      " {id: $from_id}) MATCH (b:" + to_label +
                      " {id: $to_id}) MERGE (a)-[:" + rel_type + "]->(b)";
    query_cache[cache_key] = query;
  }

  mg::Map params(2);
  if (data[from_fk_col].is_string()) {
    params.Insert("from_id", mg::Value(data[from_fk_col].get<std::string>()));
  } else {
    params.Insert("from_id", mg::Value(data[from_fk_col].get<int>()));
  }
  if (data[to_fk_col].is_string()) {
    params.Insert("to_id", mg::Value(data[to_fk_col].get<std::string>()));
  } else {
    params.Insert("to_id", mg::Value(data[to_fk_col].get<int>()));
  }
  client.ExecuteQuery(query, params);
}

void map_relationship_with_props(
    const json &data, char op, const std::string &from_label,
    const std::string &to_label, const std::string &rel_type,
    const std::string &from_fk_col, const std::string &to_fk_col,
    const std::vector<std::string> &prop_keys, MemgraphClient &client,
    std::unordered_map<std::string, std::string> &query_cache) {

  std::string query;
  const std::string cache_key = std::string(1, op) + "_rel_props_" +
                                from_label + "_" + rel_type + "_" + to_label;

  auto it = query_cache.find(cache_key);
  if (it != query_cache.end()) {
    query = it->second;
  } else {
    query = (op == 'd')
                ? "MATCH (a:" + from_label + " {id: $from_id})-[r:" + rel_type +
                      "]->(b:" + to_label + " {id: $to_id}) DELETE r"
                : "MATCH (a:" + from_label +
                      " {id: $from_id}) MATCH (b:" + to_label +
                      " {id: $to_id}) MERGE (a)-[r:" + rel_type +
                      "]->(b) SET r += $props";
    query_cache[cache_key] = query;
  }

  mg::Map params(op == 'd' ? 2 : 3);
  params.Insert("from_id", mg::Value(data[from_fk_col].get<int>()));
  params.Insert("to_id", mg::Value(data[to_fk_col].get<int>()));

  if (op != 'd') {
    mg::Map props(prop_keys.size());
    for (const auto &key : prop_keys) {
      if (data.contains(key) && !data[key].is_null()) {
        if (data[key].is_number_integer())
          props.Insert(key, mg::Value(data[key].get<int>()));
        else if (data[key].is_number_float())
          props.Insert(key, mg::Value(data[key].get<double>()));
        else if (data[key].is_boolean())
          props.Insert(key, mg::Value(data[key].get<bool>()));
        else
          props.Insert(key,
                       mg::Value(get_string_or_default(data, key.c_str())));
      }
    }
    params.Insert("props", mg::Value(std::move(props)));
  }
  client.ExecuteQuery(query, params);
}

// --- Main Processing Logic ---

void MessageHandler::Process(RdKafka::Message *msg,
                             MemgraphClient &memgraphClient) {
  // Caches persist between calls to Process because they are static
  static std::unordered_map<std::string, std::string> query_cache;
  static std::unordered_map<std::string, std::string> label_cache;

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

    std::string node_label;
    auto it = label_cache.find(table);
    if (it != label_cache.end()) {
      node_label = it->second;
    } else {
      node_label = to_pascal_case(table);
      label_cache[table] = node_label;
    }

    // --- MAPPING ROUTER ---
    if (table == "projects") {
      map_node(data, op, node_label, memgraphClient, query_cache);
      if (op != 'd' && data.contains("managed_by_user_id") &&
          !data["managed_by_user_id"].is_null()) {
        map_relationship(data, op, "User", "Project", "MANAGES",
                         "managed_by_user_id", "id", memgraphClient,
                         query_cache);
      }
    } else if (table == "businesses") {
      map_node(data, op, node_label, memgraphClient, query_cache);
      if (op != 'd') {
        if (data.contains("operator_user_id") &&
            !data["operator_user_id"].is_null())
          map_relationship(data, op, "User", "Business", "OPERATES",
                           "operator_user_id", "id", memgraphClient,
                           query_cache);
        if (data.contains("business_type_id") &&
            !data["business_type_id"].is_null())
          map_relationship(data, op, "Business", "BusinessType", "IS_TYPE",
                           "id", "business_type_id", memgraphClient,
                           query_cache);
        if (data.contains("business_category_id") &&
            !data["business_category_id"].is_null())
          map_relationship(data, op, "Business", "BusinessCategory",
                           "IN_CATEGORY", "id", "business_category_id",
                           memgraphClient, query_cache);
        if (data.contains("business_phase_id") &&
            !data["business_phase_id"].is_null())
          map_relationship(data, op, "Business", "BusinessPhase", "IN_PHASE",
                           "id", "business_phase_id", memgraphClient,
                           query_cache);
      }
    } else if (table == "skills") {
      map_node(data, op, node_label, memgraphClient, query_cache);
      if (op != 'd' && data.contains("category_id") &&
          !data["category_id"].is_null()) {
        map_relationship(data, op, "Skill", "SkillCategory", "IN_CATEGORY",
                         "id", "category_id", memgraphClient, query_cache);
      }
    } else if (table == "strengths") {
      map_node(data, op, node_label, memgraphClient, query_cache);
      if (op != 'd' && data.contains("category_id") &&
          !data["category_id"].is_null()) {
        map_relationship(data, op, "Strength", "StrengthCategory",
                         "IN_CATEGORY", "id", "category_id", memgraphClient,
                         query_cache);
      }
    } else if (table == "industries") {
      map_node(data, op, node_label, memgraphClient, query_cache);
      if (op != 'd' && data.contains("category_id") &&
          !data["category_id"].is_null()) {
        map_relationship(data, op, "Industry", "IndustryCategory",
                         "IN_CATEGORY", "id", "category_id", memgraphClient,
                         query_cache);
      }
    } else if (table == "ideas") {
      map_node(data, op, node_label, memgraphClient, query_cache);
      if (op != 'd' && data.contains("submitted_by_user_id") &&
          !data["submitted_by_user_id"].is_null()) {
        map_relationship(data, op, "User", "Idea", "SUBMITTED",
                         "submitted_by_user_id", "id", memgraphClient,
                         query_cache);
      }
    } else if (table == "user_posts") {
      map_node(data, op, node_label, memgraphClient, query_cache);
      if (op != 'd' && data.contains("poster_user_id") &&
          !data["poster_user_id"].is_null()) {
        map_relationship(data, op, "User", "UserPost", "CREATED",
                         "poster_user_id", "id", memgraphClient, query_cache);
      }
    } else if (table == "case_studies") { // NEW
      map_node(data, op, node_label, memgraphClient, query_cache);
      if (op != 'd' && data.contains("owner_user_id") &&
          !data["owner_user_id"].is_null()) {
        map_relationship(data, op, "User", "CaseStudy", "OWNS", "owner_user_id",
                         "id", memgraphClient, query_cache);
      }
    } else if (table == "notifications") { // NEW
      map_node(data, op, node_label, memgraphClient, query_cache);
      if (op != 'd') {
        if (data.contains("sender_user_id") &&
            !data["sender_user_id"].is_null()) {
          map_relationship(data, op, "User", "Notification", "SENT",
                           "sender_user_id", "id", memgraphClient, query_cache);
        }
        if (data.contains("receiver_user_id") &&
            !data["receiver_user_id"].is_null()) {
          map_relationship(data, op, "Notification", "User", "RECEIVED_BY",
                           "id", "receiver_user_id", memgraphClient,
                           query_cache);
        }
      }
    } else if (table == "users" || table == "regions" ||
               table == "subscriptions" || table == "skill_categories" ||
               table == "strength_categories" ||
               table == "business_categories" || table == "business_types" ||
               table == "business_phases" || table == "business_roles" ||
               table == "business_skills" || table == "business_strengths" ||
               table == "connection_types" || table == "mastermind_roles" ||
               table == "daily_activities" || table == "industry_categories") {
      // 'case_studies' and 'notifications' were removed from this list
      map_node(data, op, node_label, memgraphClient, query_cache);
    } else if (table == "user_logins") {
      if (op != 'd') {
        const std::string query =
            "MERGE (u:User {id: $user_id}) SET u.loginEmail = $login_email";
        mg::Map params(2);
        params.Insert("user_id", mg::Value(data["user_id"].get<int>()));
        params.Insert("login_email",
                      mg::Value(get_string_or_default(data, "login_email")));
        memgraphClient.ExecuteQuery(query, params);
      }
    } else if (table == "business_connections") {
      map_node(data, op, "BusinessConnection", memgraphClient, query_cache);
      if (op != 'd') {
        const std::string query =
            "MATCH (initiator:Business {id: $initiating_id}) MATCH "
            "(receiver:Business {id: $receiving_id}) MATCH "
            "(conn:BusinessConnection {id: $conn_id}) MERGE "
            "(initiator)-[:INITIATED_CONNECTION]->(conn) MERGE "
            "(conn)-[:RECEIVED_BY]->(receiver)";
        mg::Map params(3);
        params.Insert("initiating_id",
                      mg::Value(data["initiating_business_id"].get<int>()));
        params.Insert("receiving_id",
                      mg::Value(data["receiving_business_id"].get<int>()));
        params.Insert("conn_id", mg::Value(data["id"].get<int>()));
        memgraphClient.ExecuteQuery(query, params);

        if (data.contains("connection_type_id") &&
            !data["connection_type_id"].is_null()) {
          map_relationship(data, op, "BusinessConnection", "ConnectionType",
                           "HAS_TYPE", "id", "connection_type_id",
                           memgraphClient, query_cache);
        }
      }
    } else if (table == "project_regions") {
      map_relationship(data, op, "Project", "Region", "IN_REGION", "project_id",
                       "region_id", memgraphClient, query_cache);
    } else if (table == "user_skills") {
      map_relationship(data, op, "User", "Skill", "HAS_SKILL", "user_id",
                       "skill_id", memgraphClient, query_cache);
    } else if (table == "user_strengths") {
      map_relationship(data, op, "User", "Strength", "HAS_STRENGTH", "user_id",
                       "strength_id", memgraphClient, query_cache);
    } else if (table == "project_business_skills") {
      map_relationship(data, op, "Project", "BusinessSkill", "REQUIRES_SKILL",
                       "project_id", "business_skill_id", memgraphClient,
                       query_cache);
    } else if (table == "project_business_categories") {
      map_relationship(data, op, "Project", "BusinessCategory", "IN_CATEGORY",
                       "project_id", "business_category_id", memgraphClient,
                       query_cache);
    } else if (table == "daily_activity_enrolments") {
      map_relationship(data, op, "User", "DailyActivity", "ENROLLED_IN",
                       "user_id", "daily_activity_id", memgraphClient,
                       query_cache);
    } else if (table == "user_business_strengths") {
      map_relationship(data, op, "User", "BusinessStrength",
                       "HAS_BUSINESS_STRENGTH", "user_id",
                       "business_strength_id", memgraphClient, query_cache);
    } else if (table == "connection_mastermind_roles") {
      map_relationship(data, op, "BusinessConnection", "MastermindRole",
                       "HAS_MASTERMIND_ROLE", "connection_id",
                       "mastermind_role_id", memgraphClient, query_cache);
    } else if (table == "idea_votes") {
      map_relationship_with_props(data, op, "User", "Idea", "VOTED_ON",
                                  "voter_user_id", "idea_id", {"type"},
                                  memgraphClient, query_cache);
    } else if (table == "user_subscriptions") {
      map_relationship_with_props(
          data, op, "User", "Subscription", "HAS_SUBSCRIPTION", "user_id",
          "subscription_id",
          {"date_from", "date_to", "price", "total", "tax_amount", "tax_rate",
           "trial_from", "trial_to"},
          memgraphClient, query_cache);
    } else if (table == "user_daily_activity_progress") {
      map_relationship_with_props(data, op, "User", "DailyActivity",
                                  "HAS_PROGRESS_IN", "user_id",
                                  "daily_activity_id", {"progress", "date"},
                                  memgraphClient, query_cache);
    }

    std::cout << "[SUCCESS] Processed op '" << op << "' for table '" << table
              << "'" << std::endl;

  } catch (const std::exception &e) {
    throw std::runtime_error(
        std::string("Failed to process message for topic: ") +
        msg->topic_name() + " | " + e.what());
  }
}
