#ifndef MEMGRAPH_CLIENT_H
#define MEMGRAPH_CLIENT_H

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

// 3rd-party library
#include <mgclient.hpp>

/// <summary>
/// A C++ wrapper for the mgclient library.
/// This class simplifies the process of connecting to a Memgraph database and
/// executing queries. It uses RAII via a std::unique_ptr to manage the
/// connection lifecycle automatically.
/// </summary>
class MemgraphClient {
public:
  /// <summary>
  /// Constructs a MemgraphClient and establishes a connection to the database.
  /// </summary>
  /// <param name="host">The hostname or IP address of the Memgraph
  /// server.</param> <param name="port">The port on which the Memgraph server
  /// is running.</param> <exception cref="std::runtime_error">Thrown if the
  /// connection to Memgraph fails.</exception>
  MemgraphClient(const std::string &host, int port);

  /// <summary>
  /// Defaulted destructor. The std::unique_ptr member 'client' automatically
  /// handles the cleanup and disconnection from the Memgraph server.
  /// </summary>
  ~MemgraphClient() = default;

  // Disallow copy and assignment to prevent issues with resource ownership.
  MemgraphClient(const MemgraphClient &) = delete;
  MemgraphClient &operator=(const MemgraphClient &) = delete;

  /// <summary>
  /// Executes a given Cypher query with parameters. This method is intended for
  /// write operations as it discards all results returned from the server.
  /// </summary>
  /// <param name="query">The Cypher query string to be executed.</param>
  /// <param name="params">A constant reference to a map of parameters to be
  /// used in the query.</param> <exception cref="std::runtime_error">Thrown if
  /// the query execution fails.</exception>
  void ExecuteQuery(const std::string &query, const mg::Map &params);

  /// <summary>
  /// Runs a simple hardcoded query to test the connection and write
  /// permissions. It attempts to create a single test node.
  /// </summary>
  /// <exception cref="std::runtime_error">Thrown if the test query fails to
  /// execute.</exception>
  void RunTestQuery();

private:
  /// <summary>
  /// A smart pointer that owns and manages the underlying mg::Client connection
  /// object.
  /// </summary>
  std::unique_ptr<mg::Client> client;
};

#endif // MEMGRAPH_CLIENT_H
