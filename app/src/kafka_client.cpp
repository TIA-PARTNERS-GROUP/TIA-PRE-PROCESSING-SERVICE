#include "../include/kafka_client.hpp"

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

KafkaClient::~KafkaClient() {
  if (consumer) {
    consumer->close();
    delete consumer;
  }
}

void KafkaClient::Subscribe(const std::vector<std::string> &topics) {
  RdKafka::ErrorCode err = consumer->subscribe(topics);
  if (err != RdKafka::ERR_NO_ERROR) {
    throw std::runtime_error("Failed to subscribe to topics: " +
                             RdKafka::err2str(err));
  }
  std::cout << "✅ Subscribed to Kafka topic(s)." << std::endl;
}

RdKafka::Message *KafkaClient::Consume(int timeout_ms) {
  return consumer->consume(timeout_ms);
}
