#include "CSVParserUtil.h"
namespace VDMS {
class BoundingBoxQueryParser : public CSVParserUtil {
private:
  vector<string> rectangleKeys{"x", "y", "w", "h"};
  void parseRectangle(string row, string queryType, Json::Value &aquery);

public:
  VDMS::Response ParseAddBoundingBox(vector<string> row, vector<string> cols);
  // VDMS::Response ParseUpdateBoundingBox(vector<string> row, vector<string>&
  // cols);
};
}; // namespace VDMS

VDMS::Response
VDMS::BoundingBoxQueryParser::ParseAddBoundingBox(vector<string> row,
                                                  vector<string> columnNames) {
  if (row.empty() || row[0].empty()) {
    throw "please provide rectangle details";
  }

  Json::Value aquery;
  Json::Value allquery;
  std::string command_name = "AddBoundingBox";

  aquery["AddBoundingBox"]["_ref"] = 1;
  // aquery["AddBoundingBox"]["image"] = 3;

  Json::Value aqueryf;
  // aqueryf["FindImage"]["_ref"] = 3;
  bool cons = false;

  parseRectangle(row[0], "AddBoundingBox", aquery);

  for (int j = 1; j < columnNames.size(); j++) {
    const string &columnName = columnNames[j];
    const string &cellValue = row[j];

    if (cellValue.empty()) {
      continue;
    }

    if (columnName.find("prop_") != string::npos) {
      parseProperty(columnName, cellValue, command_name, aquery);
    } else if (columnName.find("cons_") != string::npos) {
      std::string find_image = "FindImage";
      parseConstraints(columnName, cellValue, find_image, aqueryf);
      cons = true;
    }
  }

  if (cons)
    allquery.append(aqueryf);

  allquery.append(aquery);
  // std::cout<<allquery<<std::endl;
  return send_to_vdms(allquery);
}

void VDMS::BoundingBoxQueryParser::parseRectangle(string row, string queryType,
                                                  Json::Value &aquery) {
  Json::Value rec;
  string::size_type start = 0;
  for (int c = 0; c < 4; c++) {
    string::size_type end = row.find(',', start);
    if (end == string::npos && c < 3) {
      throw "rectangle data should have four values";
    }
    string substr = row.substr(start, end - start);
    try {
      int intVal = stoi(substr);
      rec[rectangleKeys[c]] = intVal;
    } catch (const invalid_argument &) {
      try {
        float floatVal = stof(substr);
        rec[rectangleKeys[c]] = floatVal;
      } catch (const invalid_argument &) {
        throw "invalid datatype of the rectangle data";
      }
    }

    start = end + 1;
  }
  aquery[queryType]["rectangle"] = rec;
}

// VDMS::Response
// VDMS::BoundingBoxQueryParser::ParseUpdateBoundingBox(vector<string> row,
// vector<string>& columnNames){
//     Json::Value aquery;
//     Json::Value allquery;
//     aquery["UpdateBoundingBox"]["_ref"]=3;
//     std::string command_name="UpdateBoundingBox";
//     if(row[0]!=""){
//         parseRectangle(row[0],command_name,aquery);
//     }
//     for(int j=1;j<columnNames.size();j++){
//         if(row[j]!=""){
//             if(columnNames[j].find("prop_")!=string::npos){
//                 parseProperty(columnNames[j],row[j],command_name,aquery);
//             }
//             else if(columnNames[j].find("cons_")!=string::npos){
//                parseConstraints(columnNames[j],row[j],command_name,aquery);
//             }
//             else if(columnNames[j].find("remv_")!=string::npos){
//                 vector<string> rowvec;
//                 splitRowOnComma(row[j],rowvec);
//                 for(int v=0;v<rowvec.size();v++){
//                     aquery["UpdateBoundingBox"]["remove_props"].append(rowvec[v]);
//                 }
//             }
//         }
//     }
//     allquery.append(aquery);
//     return send_to_vdms(allquery);
// }
