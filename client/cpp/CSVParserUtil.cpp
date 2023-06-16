
#include "CSVParserUtil.h"
#include "BoundingBoxQueryParser.h"
#include "ConnectionQueryParser.h"
#include "DescriptorQueryParser.h"
#include "DescriptorSetQueryParser.h"
#include "EntityQueryParser.h"
#include "ImageQueryParser.h"
#include "VideoQueryParser.h"
#include "rapidcsv.h"
#include <jsoncpp/json/json.h>
#include <mutex>
#include <sstream>
static std::mutex barrier;
std::mutex mtx;
using namespace std;
using namespace std::chrono;
using namespace VDMS;

VDMS::CSVParserUtil::CSVParserUtil()
    : vdms_server("localhost"), vdms_port(55558),
      vdms_client(std::unique_ptr<VDMS::VDMSClient>(
          new VDMS::VDMSClient(vdms_server, vdms_port))) {
  initCommandsMap();
}

VDMS::CSVParserUtil::CSVParserUtil(const std::string &server, int port,
                                   const std::vector<string> columnNames,
                                   int index)
    : vdms_server(server), vdms_port(port), _columnNames(columnNames),
      id(index), vdms_client(std::unique_ptr<VDMS::VDMSClient>(
                     new VDMS::VDMSClient(vdms_server, vdms_port))) {
  initCommandsMap();
}
void VDMS::CSVParserUtil::initCommandsMap() {
  commands = {{"EntityClass", EntityClass},
              {"ConnectionClass", ConnectionClass},
              {"ImagePath", ImagePath},
              {"VideoPath", VideoPath},
              {"DescriptorType", DescriptorType},
              {"DescriptorClass", DescriptorClass},
              {"RectangleBound", RectangleBound},
              {"EntityUpdate", EntityUpdate},
              {"ConnectionUpdate", ConnectionUpdate},
              {"ImageUpdate", ImageUpdate},
              {"RectangleUpdate", RectangleUpdate}};
}
string VDMS::CSVParserUtil::function_accessing_columnNames(int i) {
  std::lock_guard<std::mutex> lock(mtx);

  return _columnNames[i];
}
VDMS::Response VDMS::CSVParserUtil::parse_row(std::vector<std::string> &row) {
  VDMS::Response result;

  VDMS::CSVParserUtil::commandType queryType = get_query_type(_columnNames[0]);
  switch (queryType) {
  case commandType::AddEntity: {
    EntityQueryParser entityquery;
    result = entityquery.ParseAddEntity(row, _columnNames);
  }

  break;
  case commandType::AddConnection: {

    ConnectionQueryParser connectionquery;
    result = connectionquery.ParseAddConnection(row, _columnNames);
  } break;

  case commandType::AddImage: {
    ImageQueryParser imagequery;
    result = imagequery.ParseAddImage(row, _columnNames);
  } break;
  case commandType::AddVideo: {
    VideoQueryParser videoquery;
    result = videoquery.ParseAddVideo(row, _columnNames);
  } break;
  case commandType::AddDescriptorSet: {
    DescriptorSetQueryParser descriptorsetquery;

    result = descriptorsetquery.ParseAddDescriptorSet(row, _columnNames);
  } break;
  case commandType::AddDescriptor: {
    DescriptorQueryParser descriptorquery;
    result = descriptorquery.ParseAddDescriptor(row, _columnNames, id);
  } break;
  case commandType::AddBoundingBox: {
    BoundingBoxQueryParser boundingboxquery;
    result = boundingboxquery.ParseAddBoundingBox(row, _columnNames);
  } break;
  // case commandType::UpdateEntity:{
  //     EntityQueryParser update_entityquery;
  //     update_entityquery.ParseUpdateEntity(row, _columnNames);
  // }
  // break;
  // case commandType::UpdateConnection:{
  //     ConnectionQueryParser update_connectionquery;
  //     update_connectionquery.ParseUpdateConnection(row,allquery,i+1,_columnNames);
  // }
  // break;
  // case commandType::UpdateImage:{
  //     ImageQueryParser update_image_query;
  //     update_image_query.ParseUpdateImage(row,allquery,i+1,_columnNames);
  // }
  //  break;
  //  case commandType::UpdateBoundingBox:{
  //     BoundingBoxQueryParser update_boundingboxquery;
  //     update_boundingboxquery.ParseUpdateBoundingBox(row,allquery,i+1,_columnNames);
  //  }
  // break;
  case commandType::UNKNOWN: {
    Json::Value results;
    results["status"] = -1;
    results["info"] = "Command does not exist";
    result.json = results.toStyledString();
  } break;
  }
  return result;
}

int VDMS::CSVParserUtil::isBool(const std::string &s) {
  std::string lower = s;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
  if (lower == "true")
    return 1;
  else if (lower == "false")
    return 2;
  return 0;
}

bool VDMS::CSVParserUtil::isFloat(const std::string &s) {
  std::istringstream ss(s);
  float x;
  char c;
  return (ss >> x) && !(ss >> c);
}

bool VDMS::CSVParserUtil::isInt(const std::string &s) {
  std::istringstream ss(s);
  int x;
  char c;
  return (ss >> x) && (floor(x)) && !(ss >> c);
}

VDMS::CSVParserUtil::commandType
VDMS::CSVParserUtil::get_query_type(const string &str) {
  CSVParserUtil::commandType querytype = commandType::UNKNOWN;

  std::lock_guard<std::mutex> lock(CSVParserUtil::querytype_mutex);
  std::map<std::string, QueryType>::iterator iter;
  iter = commands.find(str);
  if (iter != commands.end()) {
    switch (commands[str]) {
    case EntityClass:
      querytype = commandType::AddEntity;
      break;
    case ConnectionClass:
      querytype = commandType::AddConnection;
      break;
    case ImagePath:
      querytype = commandType::AddImage;
      break;
    case VideoPath:
      querytype = commandType::AddVideo;
      break;
    case DescriptorType:
      querytype = commandType::AddDescriptorSet;
      break;
    case DescriptorClass:
      querytype = commandType::AddDescriptor;
      break;
    case RectangleBound:
      querytype = commandType::AddBoundingBox;
      break;
    }
    //   std::cout << " I executed queryType "<< querytype << std::endl;
    // return querytype;
  }

  return querytype;
}

VDMS::CSVParserUtil::DATATYPE
VDMS::CSVParserUtil::getDataType(const string &str, const string &propname) {
  if (propname.substr(0, 5) == "date:")
    return DATATYPE::DATE;
  else if (isInt(str))
    return DATATYPE::INTEGER;
  else if (isFloat(str))
    return DATATYPE::FLOAT;
  else if (isBool(str) == 1)
    return DATATYPE::TRUE;
  else if (isBool(str) == 2)
    return DATATYPE::FALSE;
  else
    return DATATYPE::STRING;
}

void VDMS::CSVParserUtil::parseProperty(const string &columnNames,
                                        const string &row,
                                        const string &queryType,
                                        Json::Value &aquery) {
  std::lock_guard<std::mutex> lock(CSVParserUtil::aquery_mutex);
  string propname = columnNames.substr(5, string::npos);
  // std::cout << "Inside parseProp " << propname <<std::endl;
  CSVParserUtil::DATATYPE dtype = getDataType(row, propname);
  if (dtype == DATATYPE::DATE) {
    Json::Value date;
    date["_date"] = row;
    aquery[queryType]["properties"][propname.substr(5, string::npos)] = date;
  } else if (dtype == DATATYPE::TRUE) {
    aquery[queryType]["properties"][propname] = true;
  } else if (dtype == DATATYPE::FALSE) {
    aquery[queryType]["properties"][propname] = false;
  } else if (dtype == DATATYPE::INTEGER) {
    aquery[queryType]["properties"][propname] = stoi(row);
  } else if (dtype == DATATYPE::FLOAT) {
    aquery[queryType]["properties"][propname] = stof(row);
  } else if (dtype == DATATYPE::STRING) {
    aquery[queryType]["properties"][propname] = row;
  }
}

void VDMS::CSVParserUtil::parseConstraints(const string &columnNames,
                                           const string &row, string &queryType,
                                           Json::Value &aquery) {
  std::lock_guard<std::mutex> lock(CSVParserUtil::cons_mutex);
  vector<string> consvals = spiltrow(row);
  string consname = consvals[0];
  if (consname.substr(0, 5) == "date:") {
    for (int z = 1; z < consvals.size(); z++) {
      if (z % 2 == 1) {
        aquery[queryType]["constraints"][consname.substr(5, string::npos)]
            .append(consvals[z]);
      } else {
        Json::Value date;
        date["_date"] = consvals[z];
        aquery[queryType]["constraints"][consname.substr(5, string::npos)]
            .append(date);
      }
    }
  } else {
    for (int z = 1; z < consvals.size(); z++) {
      CSVParserUtil::DATATYPE dtype = getDataType(consvals[z], consname);
      if (dtype == DATATYPE::TRUE) {
        aquery[queryType]["constraints"][consname].append(true);
      } else if (dtype == DATATYPE::FALSE) {
        aquery[queryType]["constraints"][consname].append(false);
      } else if (dtype == DATATYPE::INTEGER) {
        aquery[queryType]["constraints"][consname].append(stoi(consvals[z]));
      } else if (dtype == DATATYPE::FLOAT) {
        aquery[queryType]["constraints"][consname].append(stof(consvals[z]));
      } else {
        aquery[queryType]["constraints"][consname].append(consvals[z]);
      }
    }
  }
}

vector<string> VDMS::CSVParserUtil::spiltrow(const string &str) {
  string row = str;
  vector<string> rowv;
  int start = 0;
  for (int i = 0; i < row.size(); i++) {
    if ((row[i] == '<') || (row[i] == '>') || (row[i] == '=')) {
      if (row[i + 1] == '=') {
        rowv.push_back(row.substr(start, i - start));
        rowv.push_back(row.substr(i, 2));
        i++;
      } else {
        rowv.push_back(row.substr(start, i - start));
        rowv.push_back(row.substr(i, 1));
      }
      start = i + 1;
    } else if (row[i] == ',') {
      row.erase(i, 1);
      i--;
    }
  }
  rowv.push_back(row.substr(start, string::npos));
  return rowv;
}
void VDMS::CSVParserUtil::parseOperations(string columnNames, string row,
                                          string queryType,
                                          Json::Value &aquery) {

  std::lock_guard<std::mutex> lock(CSVParserUtil::ops_mutex);
  string type = columnNames.substr(4, string::npos);
  Json::Value opsjson;
  vector<string> opsKeys;
  if (isValidOpsType(type))
    opsjson["type"] = type;
  else
    throw "invalid operation command name";
  vector<string> rowvec;
  int c;
  splitRowOnComma(row, rowvec);
  if (type == "crop") {
    if (rowvec.size() <= 3 || rowvec.size() > 4)
      throw "For crop data should be of size 4";
    opsKeys = {"x", "y", "width", "height"};
    for (c = 0; c < rowvec.size(); c++) {
      string substr = rowvec[c];
      DATATYPE dType = isValidDataType(substr, 2);

      if (dType == DATATYPE::INTEGER)
        opsjson[opsKeys[c]] = stoi(substr);
      else if (dType == DATATYPE::FLOAT)
        opsjson[opsKeys[c]] = stof(substr);

      else {
        throw "Numeric data is required for crop command";
      }
    }
  }

  else if (type == "threshold") {
    DATATYPE dType = isValidDataType(row, 2);
    if (dType == DATATYPE::INTEGER)
      opsjson["value"] = stoi(row);
    else if (dType == DATATYPE::FLOAT)
      opsjson["value"] = stof(row);

    else {
      throw "Numeric data is required for threshold command";
    }
  }

  else if (type == "resize") {
    if (rowvec.size() <= 1 || rowvec.size() > 2)
      throw "For resize data should be of size 2";
    opsKeys = {"width", "height"};
    for (c = 0; c < rowvec.size(); c++) {
      string substr = rowvec[c];
      DATATYPE dType = isValidDataType(substr, 2);

      if (dType == DATATYPE::INTEGER)
        opsjson[opsKeys[c]] = stoi(substr);
      else if (dType == DATATYPE::FLOAT)
        opsjson[opsKeys[c]] = stof(substr);

      else {
        throw "Numeric data is required for resize command";
      }
    }
  }

  else if (type == "flip") {
    DATATYPE dType = isValidDataType(row, 2);
    if (dType == DATATYPE::INTEGER) {
      opsjson["code"] = stoi(row);
    } else {
      throw "Numeric data is required for flip command";
    }
  }

  else if (type == "rotate") {
    if (rowvec.size() <= 1 || rowvec.size() > 2)
      throw "For rotate data should be of size 2";
    opsKeys = {"angle", "resize"};
    for (c = 0; c < rowvec.size(); c++) {
      string substr = rowvec[c];
      if (c == 0) {
        DATATYPE dType = isValidDataType(substr, 2);
        if (dType == DATATYPE::INTEGER)
          opsjson[opsKeys[c]] = stoi(substr);
        else if (dType == DATATYPE::FLOAT)
          opsjson[opsKeys[c]] = stof(substr);

        else {
          throw "Numeric data is required for rotate angle";
        }
      } else if (c == 1) {
        DATATYPE dType = isValidDataType(substr, 1);
        if (dType == DATATYPE::TRUE)
          opsjson[opsKeys[c]] = true;
        else if (dType == DATATYPE::FALSE)
          opsjson[opsKeys[c]] = false;

        else {
          throw "Boolean data is required for rotate resize";
        }
      }
    }
  }

  else if (type == "interval") {
    if (rowvec.size() <= 2 || rowvec.size() > 3)
      throw "interval operation has 3 values to specify";
    opsKeys = {"start", "stop", "step"};
    for (c = 0; c < rowvec.size(); c++) {
      string substr = rowvec[c];
      DATATYPE dType = isValidDataType(substr, 2);
      if (dType == DATATYPE::INTEGER) {
        opsjson[opsKeys[c]] = stoi(substr);
      } else {
        throw "Numeric datatype is required for the interval";
      }
    }
  }

  aquery[queryType]["operations"].append(opsjson);
}

void VDMS::CSVParserUtil::splitRowOnComma(const std::string &row,
                                          std::vector<std::string> &rowvec) {
  std::string::size_type start = 0;
  while (start != std::string::npos) {
    std::string::size_type end = row.find(',', start);
    if (end == std::string::npos) {
      if (start < row.size()) {
        rowvec.emplace_back(std::move(row.substr(start)));
      }
      break;
    }
    if (end > start) {
      rowvec.emplace_back(std::move(row.substr(start, end - start)));
    }
    start = end + 1;
  }
}

VDMS::CSVParserUtil::DATATYPE VDMS::CSVParserUtil ::isValidDataType(string data,
                                                                    int type) {
  CSVParserUtil::DATATYPE actualtype = getDataType(data, "");
  return actualtype;

  // if(type==2 && (actualt=3 || acype=tualtype==4))//2 is for num
  //     return actualtype;
  // else if(type==1 && (actualtype==1 || actualtype==2))//1 is for bool
  //     return actualtype;
  // else
  //     return -1;
}

bool VDMS::CSVParserUtil::isValidOpsType(string &type) {
  if (type == "resize" || type == "threshold" || type == "flip" ||
      type == "rotate" || type == "interval" || type == "crop")
    return true;
  return false;
}

void VDMS::CSVParserUtil::parseBlobFile(const std::string &filename,
                                        std::string **descriptor_ptr) {
  std::vector<float> v;

  std::ifstream input(filename);
  if (!input.is_open()) {
    // handle error if file cannot be opened
    std::cerr << "Error: Could not open file " << filename << std::endl;
    *descriptor_ptr = nullptr;
    return;
  }

  std::string str;
  input >> str;

  std::stringstream ss(str);
  while (ss.good()) {
    std::string substr;
    getline(ss, substr, ';');
    v.push_back(std::stof(substr));
  }

  input.close();

  // Convert vector to array of bytes
  const size_t byteSize = v.size() * sizeof(float);
  unsigned char *bytes = new unsigned char[byteSize];
  std::memcpy(bytes, v.data(), byteSize);

  // Copy bytes to dynamically allocated string
  *descriptor_ptr = new std::string(reinterpret_cast<char *>(bytes), byteSize);

  // Clean up dynamically-allocated memory
  delete[] bytes;
}

void VDMS::CSVParserUtil::videoToString(const std::string &filename,
                                        std::string **video_data_ptr) {
  // Open the video file in binary mode
  std::ifstream file(filename, std::ios::binary);

  if (!file) {
    std::cerr << "Failed to open file: " << filename << std::endl;
    *video_data_ptr = nullptr;
  } else {
    // Read the entire content of the file into a string
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string video_data = buffer.str();

    *video_data_ptr = new std::string(video_data);
  }

  // Close the file
  file.close();
}

void VDMS::CSVParserUtil::read_blob_image(const std::string &filename,
                                          std::string **image_data_ptr) {
  std::ifstream file(filename, std::ios::binary);

  if (file.is_open()) {

    // Get the size of the file
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    // Allocate a buffer to hold the file data
    char *buffer = new char[size];

    // Read the file data into the buffer
    file.read(buffer, size);

    if (file.gcount() != size) {
      std::cerr << "Error: Failed to read entire file." << std::endl;
      delete[] buffer;
      *image_data_ptr = nullptr;
      return;
    }

    // Close the file
    file.close();

    // Allocate a new std::string to hold the image data
    std::string *image_data = new std::string;
    image_data->assign(buffer, size);

    // Free the buffer
    delete[] buffer;

    // Assign the std::string pointer to the image_data_ptr
    *image_data_ptr = image_data;
  } else {
    std::cerr << "Error: Failed to open file." << std::endl;
    *image_data_ptr = nullptr;
  }
}
VDMS::Response
VDMS::CSVParserUtil::send_to_vdms(const Json::Value &query,
                                  const std::vector<std::string *> blobs) {
  Json::StyledWriter _fastwriter;

  return vdms_client->query(_fastwriter.write(query), blobs);
}
