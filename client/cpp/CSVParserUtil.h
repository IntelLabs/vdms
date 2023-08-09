#pragma once
#include "VDMSClient.h"
#include "rapidcsv.h"
#include <bits/stdc++.h>
#include <cmath>
#include <cstring>
#include <iostream>
#include <jsoncpp/json/json.h>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using namespace std::chrono;

namespace VDMS {
class CSVParserUtil {

  enum QueryType {
    EntityClass,
    ConnectionClass,
    ImagePath,
    VideoPath,
    DescriptorType,
    DescriptorClass,
    RectangleBound,
    EntityUpdate,
    ConnectionUpdate,
    ImageUpdate,
    RectangleUpdate
  };
  enum class commandType {
    AddEntity,
    AddConnection,
    AddImage,
    AddVideo,
    AddDescriptorSet,
    AddDescriptor,
    AddBoundingBox,
    UpdateEntity,
    UpdateConnection,
    UpdateImage,
    UpdateBoundingBox,
    UNKNOWN

  };
  enum class DATATYPE {
    TRUE,
    FALSE,
    INTEGER,
    FLOAT,
    STRING,
    DATE,
    WRONG

  };
  std::map<std::string, QueryType> commands;
  std::map<std::string, commandType> command_list;

public:
  CSVParserUtil();
  CSVParserUtil(const std::string &, int port, const std::vector<string>,
                int id);
  void initCommandsMap();
  int isBool(const string &data);
  bool isFloat(const string &);
  bool isInt(const string &);
  VDMS::Response parse_row(std::vector<std::string> &fields);
  commandType get_query_type(const string &data);
  CSVParserUtil::DATATYPE getDataType(const string &data,
                                      const string &colname);
  void parseProperty(const string &columnNames, const string &row,
                     const string &queryType, Json::Value &aquery);
  void parseConstraints(const string &columnNames, const string &row,
                        string &queryType, Json::Value &aquery);
  void parseOperations(const string columnNames, string row, string queryType,
                       Json::Value &aquery);

  // void parseCSVdata(int startrowcount,int endcount,rapidcsv::Document &doc,
  // int &);
  vector<string> spiltrow(const string &row);
  bool isValidOpsType(string &type);
  void splitRowOnComma(const std::string &row,
                       std::vector<std::string> &rowvec);

  string function_accessing_columnNames(int i);
  DATATYPE isValidDataType(string data, int type);
  void read_blob_image(const std::string &filename,
                       std::string **image_data_ptr);
  void videoToString(const std::string &filename, std::string **video_data);
  void parseBlobFile(const std::string &filename, std::string **descriptor_ptr);

  VDMS::Response send_to_vdms(const Json::Value &json_query,
                              const std::vector<std::string *> blobs = {});

public:
  std::string vdms_server;
  int vdms_port;
  std::vector<std::string> _columnNames;
  std::mutex querytype_mutex;
  std::mutex aquery_mutex;
  std::mutex cons_mutex;
  std::mutex ops_mutex;
  int id;
  std::unique_ptr<VDMS::VDMSClient> vdms_client;
};
}; // namespace VDMS
