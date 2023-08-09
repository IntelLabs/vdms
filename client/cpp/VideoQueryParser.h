#pragma once
#include "CSVParserUtil.h"
namespace VDMS {
class VideoQueryParser : public CSVParserUtil {
public:
  VDMS::Response ParseAddVideo(vector<string> row, vector<string> &columnNames);
  bool isValidCodec(string &row);
  bool isValidContainer(string &row);
};
} // namespace VDMS
VDMS::Response
VDMS::VideoQueryParser::ParseAddVideo(vector<string> row,
                                      vector<string> &columnNames) {
  Json::Value aquery;
  Json::Value fullquery;
  std::vector<std::string *> blobs;
  if (row[0] == "")
    throw "Video not provided";
  std::string command_name = "AddVideo";

  std::string video_name = row[0];
  try {
    std::string *video_data_ptr;
    CSVParserUtil::videoToString(video_name, &video_data_ptr);

    if (video_data_ptr != nullptr) {
      blobs.push_back(video_data_ptr);
      // std::cout <<*blobs[0] <<std::endl;
    }

    for (int j = 1; j < columnNames.size(); j++) {
      if (!row[j].empty()) {
        if (columnNames[j].find("prop_") != string::npos) {
          parseProperty(columnNames[j], row[j], command_name, aquery);
        }

        if (columnNames[j].find("ops_") != string::npos) {
          parseOperations(columnNames[j], row[j], command_name, aquery);
        }

        if (columnNames[j] == "compressto") {
          if (!isValidCodec(row[j]))
            throw "Invalid codec value";
          aquery["AddVideo"]["codec"] = row[j];
        }

        if (columnNames[j] == "format") {
          if (!isValidContainer(row[j]))
            throw "Invalid container value";
          aquery["AddVideo"]["container"] = row[j];
        }

        if (columnNames[j] == "fromserver") {
          aquery["AddVideo"]["from_server_file"] = row[j];
        }

        if (columnNames[j] == "frameindex") {
          if (row[j] == "true") {
            aquery["AddVideo"]["index_frames"] = true;
          } else if (row[j] == "false") {
            aquery["AddVideo"]["index_frames"] = false;
          } else {
            aquery["AddVideo"]["index_frames"] =
                false; // or set to a default value
          }
        }
      }
    }
    fullquery.append(aquery);

    return send_to_vdms(fullquery, blobs);
  } catch (const char *msg) {
    std::cerr << "Error: " << msg << std::endl;
  }
}

bool VDMS::VideoQueryParser::isValidCodec(string &row) {
  return (row == "xvid" || row == "h264" || row == "h263");
}

bool VDMS::VideoQueryParser::isValidContainer(string &row) {
  return (row == "mp4" || row == "avi" || row == "mov");
}
