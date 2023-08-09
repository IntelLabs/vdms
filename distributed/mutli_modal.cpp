
#include "helpers.h"
#include "kafka_receiver.h"
#include "kafka_sender.h"

int main(int argc, char *argv[]) {

  std::cout << "multi-modal" << std::endl;
  std::vector<std::unique_ptr<KafkaReceiver>> receivers;
  std::vector<std::unique_ptr<KafkaSender>> senders;

  std::unique_ptr<KafkaReceiver> receiver_image_1;
  std::unique_ptr<KafkaSender> sender_image_1;
  std::unique_ptr<KafkaReceiver> receiver_desc;
  std::unique_ptr<KafkaSender> sender_desc;
  std::unique_ptr<KafkaReceiver> receiver_meta;
  std::unique_ptr<KafkaSender> sender_meta;

  std::string package_image_type_ = "Blob";
  std::string topic_image_1 = "Image1-1-1-1-1";
  std::string topic_desc_1 = "desc-110-1-1";
  std::string topic_meta_1 = "meta-1-1-1-1";

  sender_image_1 = std::make_unique<KafkaSender>(sender_endpoint);
  receiver_image_1 = std::make_unique<KafkaReceiver>(sender_endpoint);
  receiver_image_1->Init();
  sender_image_1->Init();

  sender_desc = std::make_unique<KafkaSender>(sender_endpoint);
  receiver_desc = std::make_unique<KafkaReceiver>(sender_endpoint);
  receiver_desc->Init();
  sender_desc->Init();

  sender_meta = std::make_unique<KafkaSender>(sender_endpoint);
  receiver_meta = std::make_unique<KafkaReceiver>(sender_endpoint);
  receiver_meta->Init();
  sender_meta->Init();

  std::string set_name = "feature_set_test11_new";

  int a = 0;
  Json::Value result = construct_query();

  std::string q = writer.write(result);

  std::string msg_meta = query_body(q);
  std::string msg_img = img_query();
  std::string msg_Desc = send_descriptors(true, set_name);
  std::unique_ptr<std::stringstream> ret;
  std::string str;

  while (true) {

    if (a >= 10000)
      break;

    sender_image_1->Send(msg_img, topic_image_1, MAGENTA);
    send_to_vdms(vdms_server2, 55561,
                 (receiver_image_1->Receive(topic_image_1, CYAN))->str());
    sender_desc->Send(msg_Desc, topic_desc_1, BLUE);
    send_to_vdms(vdms_server2, 55561,
                 (receiver_desc->Receive(topic_desc_1, GREEN))->str());
    sender_meta->Send(msg_meta, topic_meta_1, BLUE);
    send_to_vdms(vdms_server2, 55561,
                 (receiver_meta->Receive(topic_meta_1, GREEN))->str());
    a++;
  }
  return 0;
}