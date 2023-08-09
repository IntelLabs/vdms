
#ifndef KAFKA_RECIVER
#define KAFKA_RECIVER

#include <glog/logging.h>
#include <iostream>
#include <librdkafka/rdkafkacpp.h>
#include <map>
#include <vector>
//#include "utils/hash_utils.h"

#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/writer.h>

#include "utils.h"
// LOG::FLAGS_minloglevel = 100;

class BaseReceiver {
public:
  BaseReceiver() {}
  virtual ~BaseReceiver() {}
  virtual bool Init() = 0;
  virtual std::unique_ptr<std::stringstream>
  Receive(const std::string &aux = "", const std::string &color = WHITE) = 0;
};

class KafkaReceiver : public BaseReceiver {
public:
  long duration;

  KafkaReceiver(const std::string &endpoint)
      : conf_(RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL)),
        tconf_(RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC)) {
    std::string errstr;

    conf_->set("bootstrap.servers", endpoint, errstr);
    conf_->set("message.max.bytes", "1000000000", errstr);
    // conf_->set("batch.size", "1048576", errstr);
    conf_->set("auto_offset_reset", "latest", errstr);
    conf_->set("socket.send.buffer.bytes", "1000000000", errstr);
    conf_->set("socket.receive.buffer.bytes", "1000000000", errstr);
    conf_->set("group.id", "auto_replication", errstr);
    conf_->set("fetch.message.max.bytes", "1000000000", errstr);
    conf_->set("socket.receive.message.max.bytes", "209715200", errstr);
  }
  virtual ~KafkaReceiver() {}
  virtual bool Init() {
    std::string errstr;
    consumer_.reset(RdKafka::Consumer::create(conf_.get(), errstr));

    return consumer_ != nullptr;
  }
  virtual std::unique_ptr<std::stringstream>
  // std::stringstream
  Receive(const std::string &aux, const std::string &color = WHITE) {
    if (consumer_.get() == nullptr) {
      LOG(FATAL) << color << "Kafka consumer was not initialized.";
    }
    std::string errstr;
    std::string topic_str = "vdms" + (aux.empty() ? "" : "-" + aux);
    std::cout << " Topic " << topic_str << std::endl;

    if (topics_.find(topic_str) == topics_.end()) {
      topics_[topic_str] =
          std::unique_ptr<RdKafka::Topic>(RdKafka::Topic::create(
              consumer_.get(), topic_str, tconf_.get(), errstr));
      if (topics_[topic_str].get() == nullptr) {
        LOG(FATAL) << color << "Failed to create topic: " << errstr;
      }
      std::cout << topics_[topic_str].get() << std::endl;
      //                                           RdKafka::Topic::OFFSET_BEGINNING);
      RdKafka::ErrorCode resp = consumer_->start(
          topics_[topic_str].get(), 0, RdKafka::Topic::OFFSET_BEGINNING);

      if (resp != RdKafka::ERR_NO_ERROR) {
        LOG(INFO) << resp
                  << " \t Kafka consume failed: " << RdKafka::err2str(resp);
      }
    }
    std::unique_ptr<std::stringstream> ret;
    while (true) {

      RdKafka::Message *msg =
          consumer_->consume(topics_[topic_str].get(), 0, 10000);

      if (msg->err() == RdKafka::ERR_NO_ERROR) {

        // LOG(INFO) << color <<"Kafka reads message at offset " <<
        // msg->offset();
        LOG(INFO) << color << "Receiver  "
                  << " \treceived " << static_cast<int>(msg->len())
                  << " bytes and storing in \t" << topic_str << std::endl;
        ;
        std::string str((char *)msg->payload(), msg->len());

        ret.reset(new std::stringstream(str));

        delete msg;

        break;
      }

      delete msg;
    }
    consumer_->poll(0);

    return ret;
  }

private:
  std::unique_ptr<RdKafka::Conf> conf_;
  std::unique_ptr<RdKafka::Conf> tconf_;
  std::unique_ptr<RdKafka::Consumer> consumer_;
  std::map<std::string, std::unique_ptr<RdKafka::Topic>> topics_;
};

#endif
