#include <iostream>

#include "../include/memgraph_client.hpp"

/// <summary>
/// Constructs a MemgraphClient and establishes a connection to the database.
/// </summary>
/// <param name="host">The hostname or IP address of the Memgraph
/// server.</param> <param name="port">The port on which the Memgraph server is
/// running.</param> <exception cref="std::runtime_error">Thrown if the
/// connection to Memgraph fails.</exception>
MemgraphClient::MemgraphClient(const std::string &host, int port) {
  mg::Client::Params params;
  params.host = host;
  params.port = port;

  client = mg::Client::Connect(params);
  if (!client) {
    throw std::runtime_error("Failed to connect to Memgraph at " + host + ":" +
                             std::to_string(port));
  }
  std::cout << "Connection to Memgraph successful!" << std::endl;
}

/// <summary>
/// Executes a given Cypher query with parameters.
/// This method is intended for write operations or when results do not need to
/// be processed, as it discards all results returned from the server.
/// </summary>
/// <param name="query">The Cypher query string to be executed.</param>
/// <param name="params">A constant reference to a map of parameters to be used
/// in the query.</param> <exception cref="std::runtime_error">Thrown if the
/// query execution fails.</exception>
void MemgraphClient::ExecuteQuery(const std::string &query,
                                  const mg::Map &params) {
  if (!client->Execute(query, params.AsConstMap())) {
    throw std::runtime_error("Failed to execute Memgraph query.");
  }
  // Discard any potential results to clear the stream for the next query.
  client->DiscardAll();
}

/// <summary>
/// Runs a simple hardcoded query to test the connection and write permissions.
/// It attempts to create a single test node.
/// </summary>
/// <exception cref="std::runtime_error">Thrown if the test query fails to
/// execute.</exception>
void MemgraphClient::RunTestQuery() {
  std::cout << "\n[TEST] Running a simple test query..." << std::endl;
  if (!client->Execute("CREATE (n:TestNode {property: 'hello world'})")) {
    throw std::runtime_error("Test query failed.");
  }
  client->DiscardAll();
  std::cout << "[TEST] Test query successful!" << std::endl;
}
