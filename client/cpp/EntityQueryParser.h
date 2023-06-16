#pragma once
#include "CSVParserUtil.h"
#include <mutex>

namespace VDMS {

class EntityQueryParser : public CSVParserUtil {
public:
  VDMS::Response ParseAddEntity(vector<string> row, vector<string> &cols);
  // VDMS::Response  ParseUpdateEntity(vector<string> row, vector<string> &
  // cols);
};
}; // namespace VDMS

VDMS::Response VDMS::EntityQueryParser::ParseAddEntity(vector<string> row,
                                                       vector<string> &cols) {
  Json::Value aquery;
  Json::Value fullquery;

  std::string command_name = "AddEntity";
  if (row[0].empty()) {
    throw "Entity Class not specified";
  }
  if (cols.size() == 0) {
    throw std::invalid_argument("Error: Column names vector is empty.");
  }
  aquery[command_name]["class"] = row[0];
  aquery[command_name]["_ref"] = 11;

  for (int j = 1; j < cols.size(); j++) {

    if (!row[j].empty()) {

      string columnType = cols[j].substr(0, 5);
      if (columnType == "prop_") {

        parseProperty(cols[j], row[j], command_name, aquery);
      } else if (columnType == "cons_") {

        parseConstraints(cols[j], row[j], command_name, aquery);
      }
    }
  }
  fullquery.append(aquery);

  return send_to_vdms(fullquery);
}

// VDMS::Response VDMS::EntityQueryParser::ParseUpdateEntity(vector<string> row,
// vector<string> & cols){
//     Json:: Value aquery;
//     Json::Value all_query;
//     Json::Value find_query;
//     std::string command_name="UpdateEntity";
//     if(row[0]==""){
//         throw "Entity Class not specified";
//     }
//     aquery["UpdateEntity"]["class"]=row[0];
//     int ref=10;
//     std::cout << _columnNames[0] <<std::endl;
//     // aquery["UpdateEntity"]["_ref"]=11;
//     for(int j=1;j<_columnNames.size();j++){

//         if(!row[j].empty()){
//             string columnType=_columnNames[j].substr(0,5);
//             if(columnType=="prop_"){
//                 parseProperty(_columnNames[j],row[j],command_name,aquery);
//             }
//             else if(columnType=="cons_"){
//                 parseConstraints(_columnNames[j],row[j],command_name,aquery);
//             }
//             else if(columnType=="remv_"){
//                 vector<string> rowvec;
//                 splitRowOnComma(row[j],rowvec);
//                 for(int v=0;v<rowvec.size();v++){
//                     aquery["UpdateEntity"]["remove_props"].append(rowvec[v]);
//                 }
//             }
//         }
//     }
//     all_query.append(aquery);
//     std::cout<<all_query <<std::endl;
//     return send_to_vdms(all_query);

// }
