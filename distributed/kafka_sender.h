#ifndef SENDERS_KAFKA_SENDER_H_
#define SENDERS_KAFKA_SENDER_H_

#include <iostream>
#include <map>

#include <glog/logging.h>
#include <librdkafka/rdkafkacpp.h>

#include "utils.h"

class BaseSender {
public:
  BaseSender() {}

  virtual ~BaseSender() {}
  virtual bool Init() = 0;
  virtual void Send(const std::string &str, const std::string &aux = "",
                    std::string color = WHITE) = 0;
};

class KafkaSender : public BaseSender {
public:
  KafkaSender(const std::string &endpoint)
      : conf_(RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL)),
        tconf_(RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC)) {
    std::string errstr;

    conf_->set("bootstrap.servers", endpoint, errstr);
    conf_->set("batch.size", "1048576", errstr);
    conf_->set("acks", "1", errstr);
    conf_->set("message.max.bytes", "1000000000", errstr);
    conf_->set("socket.send.buffer.bytes", "1000000000", errstr);
    conf_->set("socket.receive.buffer.bytes", "1000000000", errstr);
    conf_->set("socket.request.max.bytes", "209715200", errstr);
  }
  virtual ~KafkaSender() {
    if (producer_.get() != nullptr) {
      while (producer_->outq_len() > 0) {
        producer_->poll(1000);
      }
    }
  }
  virtual bool Init() {
    std::string errstr;
    producer_.reset(RdKafka::Producer::create(conf_.get(), errstr));
    return producer_.get() != nullptr;
  }
  virtual void Send(const std::string &str, const std::string &aux,
                    std::string color = WHITE) {
    if (producer_.get() == nullptr) {
      LOG(FATAL) << color << "\tKafka producer was not initialized.";
    }
    long duration;
    std::string errstr;
    std::string topic_str = "vdms" + (aux.empty() ? "" : "-" + aux);

    if (topics_.find(topic_str) == topics_.end()) {
      topics_[topic_str] =
          std::unique_ptr<RdKafka::Topic>(RdKafka::Topic::create(
              producer_.get(), topic_str, tconf_.get(), errstr));
    }
    if (topics_[topic_str].get() == nullptr) {
      LOG(FATAL) << color << "Failed to create topic: " << errstr;
    }

    RdKafka::ErrorCode resp = producer_->produce(
        topics_[topic_str].get(), RdKafka::Topic::PARTITION_UA,
        RdKafka::Producer::RK_MSG_COPY, const_cast<char *>(str.c_str()),
        str.size(), NULL, NULL);

    if (resp != RdKafka::ERR_NO_ERROR) {
      LOG(INFO) << color << "Kafka produce failed: " << RdKafka::err2str(resp);
    } else {
      LOG(INFO) << resp << "\tSender "
                << "\tKafka sent " << str.length() << " bytes to " << topic_str;
    }
    producer_->poll(0);
  }

private:
  std::unique_ptr<RdKafka::Conf> conf_;
  std::unique_ptr<RdKafka::Conf> tconf_;
  std::unique_ptr<RdKafka::Producer> producer_;
  std::map<std::string, std::unique_ptr<RdKafka::Topic>> topics_;
};

#endif
