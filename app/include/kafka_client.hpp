#ifndef KAFKA_CLIENT_H
#define KAFKA_CLIENT_H

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

// 3rd-party library
#include <librdkafka/rdkafkacpp.h>

class KafkaClient {
public:
  KafkaClient(const std::string &brokers, const std::string &groupId);
  ~KafkaClient();

  // Disallow copy and assign
  KafkaClient(const KafkaClient &) = delete;
  KafkaClient &operator=(const KafkaClient &) = delete;

  void Subscribe(const std::vector<std::string> &topics);
  RdKafka::Message *Consume(int timeout_ms);

private:
  RdKafka::KafkaConsumer *consumer;
};

#endif // KAFKA_CLIENT_H
