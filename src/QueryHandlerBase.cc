//
// Created by ifadams on 7/19/2023.
//

#include "QueryHandlerBase.h"
#include "ImageCommand.h"
#include "VideoCommand.h"

#include "VDMSConfig.h"
#include "timers/TimerMap.h"

using namespace VDMS;

valijson::Schema *QueryHandlerBase::_schema = new valijson::Schema;

QueryHandlerBase::QueryHandlerBase()
    : _validator(valijson::Validator::kWeakTypes) {}

// TODO create a better mechanism to cleanup queries that
// includes feature vectors and user-defined blobs
// For now, we do it for videos/images as a starting point.
void QueryHandlerBase::cleanup_query(const std::vector<std::string> &images,
                                     const std::vector<std::string> &videos) {
  try {
    for (auto &img_path : images) {
      VCL::Image img(img_path);
      bool result = img.delete_image();
      if (!result) {
        throw VCLException(UndefinedException,
                           "delete_image() failed: " + img_path);
      }
    }

    for (auto &vid_path : videos) {
      VCL::Video vid(vid_path);
      vid.delete_video();
    }
  } catch (VCL::Exception &e) {
  }
}

void QueryHandlerBase::process_connection(comm::Connection *c) {
  QueryMessage msgs(c);

  std::string timer_id;

  Json::Value timing_res;
  std::vector<std::string> timer_id_list;
  bool output_timing_info;

  output_timing_info =
      VDMSConfig::instance()->get_bool_value("print_high_level_timing", false);

  try {
    while (true) {
      TimerMap timers;
      protobufs::queryMessage response;
      protobufs::queryMessage query = msgs.get_query();

      timers.add_timestamp("e2e_query_processing");
      process_query(query, response);
      timers.add_timestamp("e2e_query_processing");

      timers.add_timestamp("msg_send");
      msgs.send_response(response);
      timers.add_timestamp("msg_send");

      if (output_timing_info) {
        timers.print_map_runtimes();
      }
    }
  } catch (comm::ExceptionComm e) {
    print_exception(e);
  }
}