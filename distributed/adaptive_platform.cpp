#include "helpers.h"
#include "kafka_receiver.h"
#include "kafka_sender.h"

int main(int argc, char *argv[]) {
  std::cout << "adaptive-multi-modal" << std::endl;
  std::vector<std::unique_ptr<KafkaReceiver>> receivers;
  std::vector<std::unique_ptr<KafkaSender>> senders;
  int num_receivers = 5;
  int num_senders = 5;
  int num_topics = 5;
  std::string topics[num_topics];
  for (int i = 0; i < num_topics; i++) {
    topics[i] = "topic_" + std::to_string(i);
    std::cout << topics[i] << std::endl;
  }
  Json::Value result = construct_query();

  std::string q = writer.write(result);

  std::string msg_meta = query_body(q);
  std::string msg;

  for (int i = 0; i < num_senders; i++) {
    senders.push_back(std::make_unique<KafkaSender>(sender_endpoint));
    senders[i]->Init();
  }
  for (int i = 0; i < num_receivers; i++) {
    receivers.push_back(std::make_unique<KafkaReceiver>(receiver_endpoint));
    receivers.at(i)->Init();
  }
  int a = 0;
  while (true) {
    // while(clock()/CLOCKS_PER_SEC-a < 2);
    if (a >= 100)
      break;
    for (int i = 0; i < num_senders; i++) {

      senders[i]->Send(msg_meta, topics[i], MAGENTA);
      msg = (receivers[i]->Receive(topics[i], CYAN))->str();
      std::cout << msg << std::endl;

      send_to_vdms(vdms_server2, 55561, msg);
    }

    a++;
  }

  return 0;
}