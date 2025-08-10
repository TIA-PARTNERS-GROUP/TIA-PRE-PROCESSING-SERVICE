#include "../include/memgraph_client.hpp"

MemgraphClient::MemgraphClient(const std::string &host, int port) {
  mg::Client::Params params;
  params.host = host;
  params.port = port;

  client = mg::Client::Connect(params);
  if (!client) {
    throw std::runtime_error("Failed to connect to Memgraph at " + host + ":" +
                             std::to_string(port));
  }
  std::cout << "✅ Connection to Memgraph successful!" << std::endl;
}

void MemgraphClient::ExecuteQuery(const std::string &query,
                                  const mg::Map &params) {
  if (!client->Execute(query, params.AsConstMap())) {
    throw std::runtime_error("Failed to execute Memgraph query.");
  }
  client->DiscardAll();
}

void MemgraphClient::RunTestQuery() {
  std::cout << "\n[TEST] Running a simple test query..." << std::endl;
  if (!client->Execute("CREATE (n:TestNode {property: 'hello world'})")) {
    throw std::runtime_error("Test query failed.");
  }
  client->DiscardAll();
  std::cout << "[TEST] Test query successful!" << std::endl;
}
