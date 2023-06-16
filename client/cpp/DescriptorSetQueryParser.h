#pragma once
#include "CSVParserUtil.h"
namespace VDMS {
class DescriptorSetQueryParser : public CSVParserUtil {
public:
  VDMS::Response ParseAddDescriptorSet(vector<string> row,
                                       vector<string> &columnNames);
  bool isValidMetric(string &metric);
  bool isValidEngine(string &engine);
};
}; // namespace VDMS
VDMS::Response VDMS::DescriptorSetQueryParser::ParseAddDescriptorSet(
    vector<string> row, vector<string> &columnNames) {
  if (row[0] == "") {
    throw "Descriptor Name not provided";
  }
  Json::Value aquery;
  Json::Value fullquery;
  std::string command_name = "AddDescriptorSet";
  aquery["AddDescriptorSet"]["name"] = row[0];

  for (int j = 1; j < columnNames.size(); j++) {
    if (!row[j].empty()) {
      if (columnNames[j].find("prop_") != string::npos) {
        parseProperty(columnNames[j], row[j], command_name, aquery);
      }
      if (columnNames[j] == "dimensions") {
        aquery["AddDescriptorSet"]["dimensions"] = stoi(row[j]);
      }
      if (columnNames[j] == "distancemetric") {
        if (!isValidMetric(row[j]))
          throw "Metric value is not valid";
        aquery["AddDescriptorSet"]["metric"] = row[j];
      }
      if (columnNames[j] == "searchengine") {
        if (!isValidEngine(row[j]))
          throw "Engine value is not valid";
        aquery["AddDescriptorSet"]["engine"] = row[j];
      }
    }
  }

  fullquery.append(aquery);
  return send_to_vdms(fullquery);
}

bool VDMS::DescriptorSetQueryParser::isValidMetric(string &metric) {
  return (metric == "L2" || metric == "IP");
}

bool VDMS::DescriptorSetQueryParser::isValidEngine(string &engine) {
  return (engine == "TileDBDense" || engine == "TileDBSparse" ||
          engine == "FaissFlat" || engine == "FaissIVFFlat");
}
