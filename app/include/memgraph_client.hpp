#ifndef MEMGRAPH_CLIENT_H
#define MEMGRAPH_CLIENT_H

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

// 3rd-party library
#include <mgclient.hpp>

class MemgraphClient {
public:
  MemgraphClient(const std::string &host, int port);
  ~MemgraphClient() = default;

  // Disallow copy and assign
  MemgraphClient(const MemgraphClient &) = delete;
  MemgraphClient &operator=(const MemgraphClient &) = delete;

  void ExecuteQuery(const std::string &query, const mg::Map &params);
  void RunTestQuery();

private:
  std::unique_ptr<mg::Client> client;
};

#endif // MEMGRAPH_CLIENT_H
