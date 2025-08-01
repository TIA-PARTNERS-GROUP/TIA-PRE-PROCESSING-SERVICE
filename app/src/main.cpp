#include <cstdlib>

#include <mgclient.h>

#include "../include/database.hpp"

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  struct ConnectionDetails mariadb_details = {
      "mariadb", "tia_dev_user", "devpassword", "dev_tia_db", NULL, 3306, 0};

  MYSQL *mariadb_connection = init_database(mariadb_details);

  if (!mariadb_connection)
    return EXIT_FAILURE;

  mysql_close(mariadb_connection);
  mysql_library_end();

  return EXIT_SUCCESS;
}
