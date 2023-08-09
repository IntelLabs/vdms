#include "CSVParserUtil.h"
namespace VDMS {
class ConnectionQueryParser : public CSVParserUtil {
public:
  VDMS::Response ParseAddConnection(vector<string> row, vector<string> &cols);
  //    VDMS::Response  ParseUpdateConnection(vector<string> row, vector<string>
  //    & cols);
};
}; // namespace VDMS

VDMS::Response
VDMS::ConnectionQueryParser::ParseAddConnection(vector<string> row,
                                                vector<string> &columnNames) {
  Json::Value aquery;
  Json::Value allquery;
  Json::Value find_query1, find_query2, find_query;

  if (row[0].empty()) {
    std::cerr << "Error: Connection Class not provided\n";
  }

  // Set command name and connection class
  const string command_name = "AddConnection";
  const string connection_class = row[0];
  int ref1 = 1, ref2 = 3;
  aquery["AddConnection"]["class"] = connection_class;

  // Parse class1 and class2 columns
  for (int i = 1; i < columnNames.size(); i++) {
    string column_name = columnNames[i];
    string column_value = row[i];
    string column_type = column_name.substr(0, 5);
    string command = "FindEntity";

    if (column_value.empty()) {
      continue;
    }

    if (column_name.find('@') != std::string::npos) {
      std::size_t at_pos = column_name.find('@');

      if (at_pos != std::string::npos) {
        // Extract the name and id substrings.
        std::string class1 = column_name.substr(0, at_pos);
        std::string class_prop = column_name.substr(at_pos + 1);

        find_query["FindEntity"]["class"] = class1;
        find_query["FindEntity"]["_ref"] = ref1++;
        find_query["FindEntity"]["constraints"][class_prop][0] = "==";
        find_query["FindEntity"]["constraints"][class_prop][1] = column_value;
      }
      allquery.append(find_query);
    }

    if (column_type == "prop_") {
      parseProperty(column_name, column_value, command_name, aquery);
    }
    find_query.clear();
  }

  // Set connection references
  aquery["AddConnection"]["ref1"] = allquery[0]["FindEntity"]["_ref"];
  aquery["AddConnection"]["ref2"] = allquery[1]["FindEntity"]["_ref"];

  allquery.append(aquery);
  // std::cout<<allquery<<std::endl;

  return send_to_vdms(allquery);
}

// VDMS::Response
// VDMS::ConnectionQueryParser::ParseUpdateConnection(vector<string> row,
// vector<string> & columnNames){
//    Json::Value aquery;
//    Json::Value aqueryf;
//    Json::Value allquery;
//     if(row[0]=="")
//         throw "Connection Class not provided";
//         std::string command_name="UpdateConnection";
//     aquery["UpdateConnection"]["class"]=row[0];
//    aquery["UpdateConnection"]["_ref"]=96;
//    aqueryf["FindConnection"]["class"]=row[0];
//    aqueryf["FindConnection"]["_ref"]=96;
//     for(int j=1;j<columnNames.size();j++){
//         if(row[j]!=""){
//             string columnType=columnNames[j].substr(0,5);
//             if(columnType=="prop_"){
//                 parseProperty(columnNames[j],row[j],command_name,aquery);
//             }
//             else if(columnType=="cons_"){
//                 parseConstraints(columnNames[j],row[j],command_name,aqueryf);
//             }
//             else if(columnType=="remv_"){
//                 vector<string> rowvec;
//                 splitRowOnComma(row[j],rowvec);
//                 for(int v=0;v<rowvec.size();v++){
//                     aquery["UpdateConnection"]["remove_props"].append(rowvec[v]);
//                 }
//             }
//         }
//     }
//     allquery.append(aqueryf);
//     allquery.append(aquery);
//     return send_to_vdms(allquery);
// }