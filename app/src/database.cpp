#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <mysql/mysql.h>

#include "../include/database.hpp"

struct Business;
struct UserSkill;

std::vector<User> fetch_all_users(MYSQL *conn) {
  const char *query = "SELECT id, first_name, last_name, contact_email, "
                      "contact_phone_no FROM users";

  if (mysql_query(conn, query)) {
    throw std::runtime_error("MySQL query failed: " +
                             std::string(mysql_error(conn)));
  }

  MYSQL_RES *result = mysql_store_result(conn);

  if (result == NULL) {
    throw std::runtime_error("mysql_store_result failed: " +
                             std::string(mysql_error(conn)));
  }

  std::vector<User> users;
  MYSQL_ROW row;

  while ((row = mysql_fetch_row(result))) {
    User user;

    user.id = row[0] ? std::stoi(row[0]) : 0;

    if (row[1])
      user.first_name = row[1];
    if (row[2])
      user.last_name = row[2];
    if (row[3])
      user.contact_email = row[3];
    if (row[4])
      user.contact_phone_no = row[4];

    users.push_back(user);
  }
  mysql_free_result(result);

  return users;
}

std::vector<Business> fetchAllBusinesses(MYSQL *conn) {
  const char *query = "SELECT id, operator_user_id, name, tagline, website, "
                      "description, address, city, business_type_id, "
                      "business_category_id, business_phase FROM businesses";

  if (mysql_query(conn, query)) {
    throw std::runtime_error("MySQL query failed for businesses: " +
                             std::string(mysql_error(conn)));
  }

  MYSQL_RES *result = mysql_store_result(conn);

  if (result == NULL) {
    throw std::runtime_error("mysql_store_result failed for businesses: " +
                             std::string(mysql_error(conn)));
  }

  std::vector<Business> businesses;
  MYSQL_ROW row;

  while ((row = mysql_fetch_row(result))) {
    Business business;

    business.id = row[0] ? std::stoi(row[0]) : 0;
    if (row[1])
      business.operator_user_id = std::stoi(row[1]);
    if (row[2])
      business.name = row[2];
    if (row[3])
      business.tagline = row[3];
    if (row[4])
      business.website = row[4];
    if (row[5])
      business.description = row[5];
    if (row[6])
      business.address = row[6];
    if (row[7])
      business.city = row[7];
    if (row[8])
      business.business_type_id = std::stoi(row[8]);
    if (row[9])
      business.business_category_id = std::stoi(row[9]);
    if (row[10])
      business.business_phase = std::stoi(row[10]);

    businesses.push_back(business);
  }
  mysql_free_result(result);

  return businesses;
}

std::vector<UserSkill> fetchAllUserSkills(MYSQL *conn) {
  const char *query = "SELECT skill_id, user_id FROM user_skills";

  if (mysql_query(conn, query)) {
    throw std::runtime_error("MySQL query failed for user_skills: " +
                             std::string(mysql_error(conn)));
  }

  MYSQL_RES *result = mysql_store_result(conn);

  if (result == NULL) {
    throw std::runtime_error("mysql_store_result failed for user_skills: " +
                             std::string(mysql_error(conn)));
  }

  std::vector<UserSkill> user_skills;

  MYSQL_ROW row;

  while ((row = mysql_fetch_row(result))) {
    UserSkill user_skill;

    user_skill.skill_id = std::stoi(row[0]);
    user_skill.user_id = std::stoi(row[1]);
    user_skills.push_back(user_skill);
  }
  mysql_free_result(result);

  return user_skills;
}

MYSQL *init_database(const ConnectionDetails &connection_details) {
  if (mysql_library_init(0, NULL, NULL)) {
    std::cerr << "Could not initialize MySQL client library." << std::endl;
    return NULL;
  }

  MYSQL *database_connection = mysql_init(NULL);

  if (database_connection == NULL) {
    std::cerr << "Error: mysql_init() failed" << std::endl;
    mysql_library_end();
    return NULL;
  }

  if (mysql_real_connect(database_connection, connection_details.server,
                         connection_details.user, connection_details.password,
                         connection_details.database, connection_details.port,
                         NULL, 0) == NULL) {

    std::cerr << "Error connecting to the database: "
              << mysql_error(database_connection) << std::endl;

    mysql_close(database_connection);
    mysql_library_end();
    return NULL; // Return NULL on failure
  }

  std::cout << "✅ Connection Successful!" << std::endl;
  return database_connection; // Return the valid connection handle
}
