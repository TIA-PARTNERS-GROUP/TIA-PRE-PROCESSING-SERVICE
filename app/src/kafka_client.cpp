#include <iostream>

#include "../include/kafka_client.hpp"

/// <summary>
/// Constructs a KafkaClient object. This involves creating and configuring
/// an underlying RdKafka::KafkaConsumer instance.
/// </summary>
/// <param name="brokers">A string containing the comma-separated list of Kafka
/// broker hostnames (e.g., "localhost:9092").</param> <param name="groupId">The
/// consumer group ID that this client will be a part of.</param> <exception
/// cref="std::runtime_error">Thrown if the RdKafka::KafkaConsumer fails to be
/// created.</exception>
KafkaClient::KafkaClient(const std::string &brokers,
                         const std::string &groupId) {
  std::string errstr;
  RdKafka::Conf *conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);

  conf->set("bootstrap.servers", brokers, errstr);
  conf->set("group.id", groupId, errstr);
  conf->set("auto.offset.reset", "earliest", errstr);

  consumer = RdKafka::KafkaConsumer::create(conf, errstr);
  delete conf;

  if (!consumer) {
    throw std::runtime_error("Failed to create Kafka consumer: " + errstr);
  }
}

/// <summary>
/// Destructor for the KafkaClient. It ensures that the underlying consumer
/// is properly closed and its resources are released.
/// </summary>
KafkaClient::~KafkaClient() {
  if (consumer) {
    consumer->close();
    delete consumer;
  }
}

/// <summary>
/// Subscribes the consumer to a list of Kafka topics.
/// </summary>
/// <param name="topics">A vector of strings, where each string is a topic
/// name.</param> <exception cref="std::runtime_error">Thrown if the
/// subscription fails for any reason.</exception>
void KafkaClient::Subscribe(const std::vector<std::string> &topics) {
  RdKafka::ErrorCode err = consumer->subscribe(topics);
  if (err != RdKafka::ERR_NO_ERROR) {
    throw std::runtime_error("Failed to subscribe to topics: " +
                             RdKafka::err2str(err));
  }
  std::cout << "Subscribed to Kafka topic(s)." << std::endl;
}

/// <summary>
/// Consumes a single message from the subscribed topics. This is a blocking
/// call.
/// </summary>
/// <param name="timeout_ms">The maximum time to wait for a message, in
/// milliseconds.</param> <returns> A pointer to a RdKafka::Message object. The
/// caller is responsible for deleting this message by calling delete on the
/// pointer. The message object may indicate an error (e.g., timeout) which
/// should be checked via its err() method.
/// </returns>
RdKafka::Message *KafkaClient::Consume(int timeout_ms) {
  return consumer->consume(timeout_ms);
}
