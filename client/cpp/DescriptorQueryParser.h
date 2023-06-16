#pragma once
#include "CSVParserUtil.h"
namespace VDMS {
class DescriptorQueryParser : public CSVParserUtil {
public:
  VDMS::Response ParseAddDescriptor(vector<string> row,
                                    vector<string> &columnNames, int id);
};
}; // namespace VDMS

VDMS::Response VDMS::DescriptorQueryParser::ParseAddDescriptor(
    vector<string> row, vector<string> &columnNames, int id) {

  if (row[0] == "") {
    throw "Set not provided";
  }
  Json::Value aquery;
  Json::Value fullquery;
  std::vector<std::string *> blobs;
  std::string *descriptor;
  std::string command_name = "AddDescriptor";
  aquery["AddDescriptor"]["set"] = row[0];
  aquery["AddDescriptor"]["_ref"] = id + 3;
  for (int j = 1; j < columnNames.size(); j++) {
    if (row[j] != "") {
      if (columnNames[j].find("prop_") != string::npos) {
        parseProperty(columnNames[j], row[j], command_name, aquery);
      }

      if (columnNames[j] == "label") {
        aquery["AddDescriptor"]["label"] = row[j];
      }
      if (columnNames[j] == "inputdata") {

        parseBlobFile(row[j], &descriptor);
        if (descriptor == nullptr) {
          std::cout << "Failed to parse blob file" << std::endl;
        }

        if (descriptor != nullptr) {
          blobs.push_back(descriptor);
        }
      }
    }
  }
  fullquery.append(aquery);
  return send_to_vdms(fullquery, blobs);
}
