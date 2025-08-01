#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <cstdint>
#include <mysql/mysql.h>
#include <vector>

#include "./models.hpp"

struct ConnectionDetails {
  const char *server;
  const char *user;
  const char *password;
  const char *database;
  const char *unix_socket;

  const int16_t port;
  const int8_t client_flag;
};

MYSQL *init_database(const ConnectionDetails &connection_details);
std::vector<User> fetch_all_users(MYSQL *database_connection);
std::vector<Business> fetch_all_businesses(MYSQL *database_connection);
std::vector<UserSkill> fetch_all_user_skills(MYSQL *database_connection);

#endif // !DATABASE_HPP
