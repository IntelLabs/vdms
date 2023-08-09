#pragma once
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/writer.h>
#include <list>
#include <regex>
#include <time.h>
#include <vector>

#include "VDMSClient.h"
#include "helpers.h"
#include "vcl/VCL.h"
#include "gtest/gtest.h"

class Meta_Data {
public:
  std::shared_ptr<VDMS::VDMSClient> _aclient;
  std::string _server_name = "localhost";
  int _port = 55558;

  Json::FastWriter _fastwriter;
  Json::Reader _reader;
  Json::Value _result;

  Meta_Data();

  Json::Value construct_add_query(int ref, bool const_on, bool experiation);
  Json::Value construct_add_area(int ref, bool const_on);
  Json::Value construct_add_connection(int ref1, int ref2, bool const_on);
  Json::Value construct_find_entity(bool, bool);
  Json::Value constuct_BB(bool);
  Json::Value construct_Blob();
  Json::Value construct_updateBlob();
  Json::Value construct_findBlob();
  std::string *read_blob(std::string &);
  Json::Value constuct_image(bool = false, Json::Value operations = {});
  Json::Value constuct_video(bool = false);
  Json::Value construct_find_image();
  Json::Value construct_find_image_withop(Json::Value operations);
  Json::Value construct_descriptor();
  Json::Value construct_find_descriptor();
  Json::Value construct_flinng_descriptor();
  Json::Value construct_find_flinng_descriptor();
  Json::Value construct_Flinng_Set(std::string &, int &);
  std::string get_server() { return _server_name; }
  int get_port() { return _port; }
};
