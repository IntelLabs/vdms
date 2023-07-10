#ifndef SENDERS_KAFKA_HELPER_H_
#define SENDERS_KAFKA_HELPER_H_
#include "VDMSClient.h"
#include "queryMessage.pb.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/writer.h>
#include <thread>

#include <glog/logging.h>

#include "utils.h"

using namespace std::chrono;
std::string sender_endpoint = "broker:19092";
std::string receiver_endpoint = "broker:19092";
std::string vdms_server1 = "localhost";
std::string vdms_server2 = "localhost";
int number_receivers = 1;
int number_senders = 1;
int vdms_port1 = 55560;
int vdms_port2 = 55561;

Json::FastWriter writer;
Json::Reader reader;
Json::Value result;

//***************************
using namespace std;

std::shared_ptr<VDMS::VDMSClient> _aclient;

VDMS::Response send_to_vdms(std::string server_url = "localhost",
                            int port = 55561, std::string msg = "") {
  std::basic_string<uint8_t> t = std::basic_string<uint8_t>(
      (const unsigned char *)msg.data(), msg.length());
  std::vector<std::string *> blobs;

  VDMS::protobufs::queryMessage proto_query;

  proto_query.ParseFromArray((const void *)t.data(), t.length());
  Json::Value root;
  Json::Reader reader;

  const std::string commands = proto_query.json();
  bool parseSuccess = reader.parse(commands.c_str(), root);
  if (!parseSuccess) {
    root["info"] = "Error parsing the query, ill formed JSON";
    root["status"] = -1;
  }
  for (auto &it : proto_query.blobs()) {
    blobs.push_back(new std::string(it));
  }

  _aclient.reset(new VDMS::VDMSClient(server_url, port));

  VDMS::Response responses = _aclient->query(commands, blobs);
  Json::Value parsed;

  reader.parse(responses.json.c_str(), parsed);
  std::cout << parsed << std::endl;
  return responses;
}

std::string query_body(std::string &query,
                       const std::vector<std::string *> blobs = {}) {
  VDMS::protobufs::queryMessage proto_query;
  proto_query.set_json(query);

  for (auto &it : blobs) {
    std::string *blob = proto_query.add_blobs();
    *blob = *it;
  }

  std::basic_string<char> msg_image(proto_query.ByteSize(), 0);
  std::cout << "Sending size " << proto_query.ByteSize() << "\t"
            << msg_image.length() << std::endl;
  msg_image[msg_image.length() - 1] = '\0';
  proto_query.SerializeToArray((void *)msg_image.data(), msg_image.length());
  std::string t(msg_image.begin(), msg_image.end());
  return t;
}

Json::Value add_set(std::string &name) {
  Json::Value descriptor_set;
  Json::Value set_query;
  Json::Value tuple;

  descriptor_set["name"] = name;
  descriptor_set["dimensions"] = 1000;
  set_query["AddDescriptorSet"] = descriptor_set;
  if (add_set)
    tuple.append(set_query);
  return tuple;
}

Json::Value construct_descriptor(std::string &name) {

  Json::Value AddDesc;
  Json::Value Desc;
  Json::Value tuple;
  Desc["set"] = name;
  Desc["label"] = "Person";
  Desc["_ref"] = 1;
  Desc["properties"]["id"] = 123;
  Desc["properties"]["name"] = "Ali";
  AddDesc["AddDescriptor"] = Desc;
  tuple.append(AddDesc);
  return tuple;
}

std::string send_descriptors(bool new_set, std::string &name) {
  std::vector<float> fv_values;
  srand((unsigned)time(NULL));

  for (int i = 0; i < 1000; i++) {
    fv_values.push_back((float)rand() / RAND_MAX);
  }
  std::vector<std::string *> blobs;
  std::string *bytes_str = new std::string();
  bytes_str->resize(fv_values.size() * sizeof(float));
  std::memcpy((void *)bytes_str->data(), fv_values.data(),
              fv_values.size() * sizeof(float));
  blobs.push_back(bytes_str);

  Json::Value desc_query = construct_descriptor(name);
  std::string add_desc = writer.write(desc_query);
  std::cout << add_desc << std::endl;
  std::string result = query_body(add_desc, blobs);
  return result;
}

Json::Value add_image() {
  Json::Value image;
  image["format"] = "png";

  Json::Value props;
  props["name"] = "Ali";
  image["properties"] = props;
  Json::Value add_image;
  add_image["AddImage"] = image;
  Json::Value tuple;
  tuple.append(add_image);
  return tuple;
}
Json::Value construct_query() {
  Json::Value person_json, bounding_box, add_bounding_box, add_FV_entity,
      add_person_entity, edge, connect, tuple_data;
  person_json["_ref"] =
      1; // to assure the differences between the used references in the DB
  person_json["class"] = "Person";
  person_json["properties"]["Id"] = "1234";
  person_json["properties"]["imaginary_node"] = 1;
  person_json["constraints"]["Id"][0] = "==";
  person_json["constraints"]["Id"][1] = "1234";
  add_person_entity["AddEntity"] = person_json;
  tuple_data.append(add_person_entity);
  add_person_entity.clear();

  bounding_box["_ref"] = 2;
  bounding_box["class"] = "BoundingBox";
  bounding_box["properties"]["Id"] = "1234";
  bounding_box["properties"]["X"] = 50;
  bounding_box["properties"]["Y"] = 50;
  bounding_box["properties"]["Width"] = 100;
  bounding_box["properties"]["Height"] = 100;
  add_bounding_box["AddEntity"] = bounding_box;
  tuple_data.append(add_bounding_box);

  // add the connection between the person and its bounding box
  edge["ref1"] = person_json["_ref"].asInt();
  edge["ref2"] = bounding_box["_ref"].asInt();
  edge["class"] = "Represents";
  connect["AddConnection"] = edge;
  tuple_data.append(connect);

  return tuple_data;
}

std::string img_query() {
  Json::Value img_query_ = add_image();
  std::string addImg = writer.write(img_query_);
  std::string image;
  std::ifstream file("../tests/test_images/brain.png",
                     std::ios::in | std::ios::binary | std::ios::ate);
  image.resize(file.tellg());

  file.seekg(0, std::ios::beg);
  if (!file.read(&image[0], image.size()))
    std::cout << "error" << std::endl;

  std::vector<std::string *> blobs;
  std::string *bytes_str = new std::string(image);
  blobs.push_back(bytes_str);
  std::string result = query_body(addImg, blobs);

  return result;
}
#endif