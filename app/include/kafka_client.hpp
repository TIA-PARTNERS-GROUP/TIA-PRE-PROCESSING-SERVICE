#ifndef KAFKA_CLIENT_H
#define KAFKA_CLIENT_H

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

// 3rd-party library
#include <librdkafka/rdkafkacpp.h>

/// <summary>
/// A C++ wrapper for the librdkafka KafkaConsumer client.
/// This class simplifies the process of creating a consumer, subscribing to
/// topics, and consuming messages. It ensures proper resource management.
/// </summary>
class KafkaClient {
public:
  /// <summary>
  /// Constructs a KafkaClient object, creating and configuring
  /// an underlying RdKafka::KafkaConsumer instance.
  /// </summary>
  /// <param name="brokers">A string containing the comma-separated list of
  /// Kafka broker hostnames (e.g., "localhost:9092").</param> <param
  /// name="groupId">The consumer group ID that this client will be a part
  /// of.</param>
  KafkaClient(const std::string &brokers, const std::string &groupId);

  /// <summary>
  /// Destructor for the KafkaClient. It ensures that the underlying consumer
  /// is properly closed and its resources are released.
  /// </summary>
  ~KafkaClient();

  // Disallow copy and assignment to prevent issues with resource ownership.
  KafkaClient(const KafkaClient &) = delete;
  KafkaClient &operator=(const KafkaClient &) = delete;

  /// <summary>
  /// Subscribes the consumer to a list of Kafka topics.
  /// </summary>
  /// <param name="topics">A vector of strings, where each string is a topic
  /// name.</param>
  void Subscribe(const std::vector<std::string> &topics);

  /// <summary>
  /// Consumes a single message from the subscribed topics. This is a blocking
  /// call.
  /// </summary>
  /// <param name="timeout_ms">The maximum time to wait for a message, in
  /// milliseconds.</param> <returns> A pointer to a RdKafka::Message object.
  /// The caller is responsible for deleting this message by calling delete on
  /// the pointer. The message object may indicate an error (e.g., timeout)
  /// which should be checked via its err() method.
  /// </returns>
  RdKafka::Message *Consume(int timeout_ms);

private:
  /// <summary>
  /// A raw pointer to the underlying librdkafka consumer instance.
  /// </summary>
  RdKafka::KafkaConsumer *consumer;
};

#endif // KAFKA_CLIENT_H
