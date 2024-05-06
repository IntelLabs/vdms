//
// Created by ifadams on 7/19/2023.
//

#include "QueryHandlerBase.h"
#include "ImageCommand.h"
#include "VideoCommand.h"

using namespace VDMS;

valijson::Schema *QueryHandlerBase::_schema = new valijson::Schema;

QueryHandlerBase::QueryHandlerBase()
    : _validator(valijson::Validator::kWeakTypes)
#ifdef CHRONO_TIMING
      ,
      ch_tx_total("ch_tx_total"), ch_tx_query("ch_tx_query"),
      ch_tx_send("ch_tx_send")
#endif
{
}

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

  try {
    while (true) {
      protobufs::queryMessage response;
      protobufs::queryMessage query = msgs.get_query();
      CHRONO_TIC(ch_tx_total);

      CHRONO_TIC(ch_tx_query);
      process_query(query, response);
      CHRONO_TAC(ch_tx_query);

      CHRONO_TIC(ch_tx_send);
      msgs.send_response(response);
      CHRONO_TAC(ch_tx_send);

      CHRONO_TAC(ch_tx_total);
      CHRONO_PRINT_LAST_MS(ch_tx_total);
      CHRONO_PRINT_LAST_MS(ch_tx_query);
      CHRONO_PRINT_LAST_MS(ch_tx_send);
    }
  } catch (comm::ExceptionComm e) {
    print_exception(e);
  }
}