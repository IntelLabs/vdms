#include "helpers.h"

#include "kafka_receiver.h"
#include "kafka_sender.h"

int main(int argc, char *argv[]) {

  Json::Value query;

  std::string package_type_;
  std::string topic_meta_1;
  std::string topic_meta_2;
  std::unique_ptr<KafkaReceiver> receiver_meta_1;
  std::unique_ptr<KafkaSender> sender_meta_1;
  std::unique_ptr<KafkaReceiver> receiver_meta_2;
  std::unique_ptr<KafkaSender> sender_meta_2;

  package_type_ = "message";
  topic_meta_1 = "query_test_1";
  topic_meta_2 = "query_test_2";

  sender_meta_1 = std::make_unique<KafkaSender>(sender_endpoint);
  receiver_meta_1 = std::make_unique<KafkaReceiver>(receiver_endpoint);

  receiver_meta_1->Init();
  sender_meta_1->Init();

  int a = clock() / CLOCKS_PER_SEC;
  std::string msg;

  std::string q = writer.write(construct_query());

  msg = query_body(q);

  while (true) {
    while (clock() / CLOCKS_PER_SEC - a < 2)
      ;
    if (a >= 100)
      break;

    sender_meta_1->Send(msg, topic_meta_1, BLUE);
    send_to_vdms(vdms_server2, 55561,
                 (receiver_meta_1->Receive(topic_meta_1, GREEN))->str());
  }

  return 0;
}
