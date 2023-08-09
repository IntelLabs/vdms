#pragma once
#include "CSVParserUtil.h"
namespace VDMS {
class ImageQueryParser : public CSVParserUtil {
private:
  std::mutex file_access_mutex;

public:
  // ImageQueryParser();
  VDMS::Response ParseAddImage(vector<string> row, vector<string> columnNames);
  // VDMS::Response ParseUpdateImage(vector<string> row,  vector<string>
  // columnNames);
  bool ValidImageFormat(string data);
};
}; // namespace VDMS

VDMS::Response
VDMS::ImageQueryParser::ParseAddImage(vector<string> row,
                                      vector<string> columnNames) {
  Json::Value aquery;
  Json::Value fullquery;
  std::vector<std::string *> blobs;
  //
  if (row[0].empty())
    throw "Image path is not specified";
  if (columnNames.size() == 0) {
    throw std::invalid_argument("Error: Column names vector is empty.");
  }

  std::string command_name = "AddImage";

  aquery["AddImage"]["_ref"] = 11;

  std::string name = row[0];

  std::string *image_data_ptr = nullptr;

  read_blob_image(name, &image_data_ptr);

  //    std::cout << *image_data_ptr << std::endl;
  if (image_data_ptr != nullptr) {
    blobs.push_back(image_data_ptr);
    // std::cout <<*blobs[0] <<std::endl;
  }

  for (int j = 1; j < columnNames.size(); j++) {
    if (!row[j].empty()) {
      if (columnNames[j] == "format") {
        if (!ValidImageFormat(row[j]))
          throw "Invalid image format";
        aquery["AddImage"]["format"] = row[j];
      } else if (columnNames[j].find("prop_") != string::npos) {
        VDMS::CSVParserUtil::parseProperty(columnNames[j], row[j], command_name,
                                           aquery);
      } else if (columnNames[j].find("ops_") != string::npos) {
        VDMS::CSVParserUtil::parseOperations(columnNames[j], row[j],
                                             command_name, aquery);
      } else if (columnNames[j].find("cons_") != string::npos) {
        VDMS::CSVParserUtil::parseConstraints(columnNames[j], row[j],
                                              command_name, aquery);
      }
    }
  }
  fullquery.append(aquery);

  //    delete image_data_ptr;
  return send_to_vdms(fullquery, blobs);
}

bool VDMS::ImageQueryParser::ValidImageFormat(string data) {
  return (data == "png" || data == "jpg" || data == "tdb" || data == "bin");
}

// VDMS::Response VDMS::ImageQueryParser::ParseUpdateImage(vector<string> row,
// vector<string> columnNames){
//     Json :: Value aquery;

//     Json::Value fullquery;

//     std::string command_name="UpdateIamge";
//     aquery["UpdateImage"]["_ref"]=12;
//     for(int j=1;j<columnNames.size();j++){
//                 if(row[j]!=""){
//                     if(columnNames[j].find("prop_")!=string::npos){
//                     parseProperty(columnNames[j],row[j],command_name,aquery);
//                     }
//                 else if(columnNames[j].find("remv_")!=string::npos){
//                     vector<string> rowvec;
//                 splitRowOnComma(row[j],rowvec);
//                 for(int v=0;v<rowvec.size();v++){
//                     aquery["UpdateImage"]["remove_props"].append(rowvec[v]);
//                 }
//                 }
//                 else if(columnNames[j].find("cons_")!=string::npos){
//                     parseConstraints(columnNames[j],row[j],command_name,aquery);
//                 }
//                 }
//             }
//             fullquery.append(aquery);
//             return send_to_vdms(fullquery);

// }