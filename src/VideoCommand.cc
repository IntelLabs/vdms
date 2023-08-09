/**
 * @file   VideoCommand.cc
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2017 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <filesystem>
#include <fstream>
#include <iostream>

#include "ImageCommand.h" // for enqueue_operations of Image type
#include "VDMSConfig.h"
#include "VideoCommand.h"
#include "defines.h"

using namespace VDMS;
namespace fs = std::filesystem;

VideoCommand::VideoCommand(const std::string &cmd_name) : RSCommand(cmd_name) {}

void VideoCommand::enqueue_operations(VCL::Video &video,
                                      const Json::Value &ops) {
  // Correct operation type and parameters are guaranteed at this point
  for (auto &op : ops) {
    const std::string &type = get_value<std::string>(op, "type");
    std::string unit;
    if (type == "threshold") {
      video.threshold(get_value<int>(op, "value"));

    } else if (type == "interval") {

      video.interval(VCL::Video::FRAMES, get_value<int>(op, "start"),
                     get_value<int>(op, "stop"), get_value<int>(op, "step"));

    } else if (type == "resize") {
      video.resize(get_value<int>(op, "height"), get_value<int>(op, "width"));

    } else if (type == "crop") {
      video.crop(VCL::Rectangle(
          get_value<int>(op, "x"), get_value<int>(op, "y"),
          get_value<int>(op, "width"), get_value<int>(op, "height")));
    } else {
      throw ExceptionCommand(ImageError, "Operation not defined");
    }
  }
}

VCL::Video::Codec VideoCommand::string_to_codec(const std::string &codec) {
  if (codec == "h263") {
    return VCL::Video::Codec::H263;
  } else if (codec == "xvid") {
    return VCL::Video::Codec::XVID;
  } else if (codec == "h264") {
    return VCL::Video::Codec::H264;
  }

  return VCL::Video::Codec::NOCODEC;
}

Json::Value VideoCommand::check_responses(Json::Value &responses) {
  if (responses.size() != 1) {
    Json::Value return_error;
    return_error["status"] = RSCommand::Error;
    return_error["info"] = "PMGD Response Bad Size";
    return return_error;
  }

  Json::Value &response = responses[0];

  if (response["status"] != 0) {
    response["status"] = RSCommand::Error;
    // Uses PMGD info error.
    return response;
  }

  return response;
}

//========= AddVideo definitions =========

AddVideo::AddVideo() : VideoCommand("AddVideo") {
  _storage_video = VDMSConfig::instance()->get_path_videos();
  //_use_aws_storage = VDMSConfig::instance()->get_aws_flag();
}

int AddVideo::construct_protobuf(PMGDQuery &query, const Json::Value &jsoncmd,
                                 const std::string &blob, int grp_id,
                                 Json::Value &error) {
  const Json::Value &cmd = jsoncmd[_cmd_name];

  int node_ref = get_value<int>(cmd, "_ref", query.get_available_reference());

  const std::string from_server_file =
      get_value<std::string>(cmd, "from_server_file", "");
  VCL::Video video;

  if (_use_aws_storage) {
    VCL::RemoteConnection *connection = new VCL::RemoteConnection();
    std::string bucket = VDMSConfig::instance()->get_bucket_name();
    connection->_bucket_name = bucket;
    video.set_connection(connection);
  }

  if (from_server_file.empty())
    video = VCL::Video((void *)blob.data(), blob.size());
  else
    video = VCL::Video(from_server_file);

  // Key frame extraction works on binary stream data, without encoding. We
  // check whether key-frame extraction is to be applied, and if so, we
  // extract the frames before any other operations are applied. Applying
  // key-frame extraction after applying pending operations will be
  // non-optimal: the video will be decoded while performing the operations.
  VCL::KeyFrameList frame_list;
  if (get_value<bool>(cmd, "index_frames", false))
    frame_list = video.get_key_frame_list();

  if (cmd.isMember("operations")) {
    enqueue_operations(video, cmd["operations"]);
  }

  // The container and codec are checked by the schema.
  // We default to mp4 and h264, if not specified
  const std::string &container =
      get_value<std::string>(cmd, "container", "mp4");
  const std::string &file_name = VCL::create_unique(_storage_video, container);

  // Modifiyng the existing properties that the user gives
  // is a good option to make the AddNode more simple.
  // This is not ideal since we are manupulating with user's
  // input, but for now it is an acceptable solution.
  Json::Value props = get_value<Json::Value>(cmd, "properties");
  props[VDMS_VID_PATH_PROP] = file_name;

  // Add Video node
  query.AddNode(node_ref, VDMS_VID_TAG, props, Json::Value());

  const std::string &codec = get_value<std::string>(cmd, "codec", "h264");
  VCL::Video::Codec vcl_codec = string_to_codec(codec);

  video.store(file_name, vcl_codec);

  if (_use_aws_storage) {
    video._remote->Write(file_name);
    std::remove(file_name.c_str()); // remove the local copy of the file
  }

  // Add key-frames (if extracted) as nodes connected to the video
  for (const auto &frame : frame_list) {
    Json::Value frame_props;
    frame_props[VDMS_KF_IDX_PROP] = static_cast<Json::UInt64>(frame.idx);
    frame_props[VDMS_KF_BASE_PROP] = static_cast<Json::Int64>(frame.base);

    int frame_ref = query.get_available_reference();
    query.AddNode(frame_ref, VDMS_KF_TAG, frame_props, Json::Value());
    query.AddEdge(-1, node_ref, frame_ref, VDMS_KF_EDGE, Json::Value());
  }

  // In case we need to cleanup the query
  error["video_added"] = file_name;

  if (cmd.isMember("link")) {
    add_link(query, cmd["link"], node_ref, VDMS_VID_EDGE);
  }

  return 0;
}

Json::Value AddVideo::construct_responses(Json::Value &response,
                                          const Json::Value &json,
                                          protobufs::queryMessage &query_res,
                                          const std::string &blob) {
  Json::Value ret;
  ret[_cmd_name] = RSCommand::check_responses(response);

  return ret;
}

bool AddVideo::need_blob(const Json::Value &cmd) {
  const Json::Value &add_video_cmd = cmd[_cmd_name];
  return !(add_video_cmd.isMember("from_server_file"));
}

//========= UpdateVideo definitions =========

UpdateVideo::UpdateVideo() : VideoCommand("UpdateVideo") {}

int UpdateVideo::construct_protobuf(PMGDQuery &query,
                                    const Json::Value &jsoncmd,
                                    const std::string &blob, int grp_id,
                                    Json::Value &error) {
  const Json::Value &cmd = jsoncmd[_cmd_name];

  int node_ref = get_value<int>(cmd, "_ref", -1);

  Json::Value constraints = get_value<Json::Value>(cmd, "constraints");

  Json::Value props = get_value<Json::Value>(cmd, "properties");

  Json::Value remove_props = get_value<Json::Value>(cmd, "remove_props");

  // Update Image node
  query.UpdateNode(node_ref, VDMS_VID_TAG, props, remove_props, constraints,
                   get_value<bool>(cmd, "unique", false));

  return 0;
}

Json::Value UpdateVideo::construct_responses(Json::Value &responses,
                                             const Json::Value &json,
                                             protobufs::queryMessage &query_res,
                                             const std::string &blob) {
  assert(responses.size() == 1);

  Json::Value ret;

  // TODO In order to support "codec" or "operations", we could
  // implement VCL save operation here.

  ret[_cmd_name].swap(responses[0]);
  return ret;
}

//========= FindVideo definitions =========

FindVideo::FindVideo() : VideoCommand("FindVideo") {
  //_use_aws_storage = VDMSConfig::instance()->get_aws_flag();
}

int FindVideo::construct_protobuf(PMGDQuery &query, const Json::Value &jsoncmd,
                                  const std::string &blob, int grp_id,
                                  Json::Value &error) {
  const Json::Value &cmd = jsoncmd[_cmd_name];

  Json::Value results = get_value<Json::Value>(cmd, "results");

  // Unless otherwhise specified, we return the blob.
  if (get_value<bool>(results, "blob", true)) {
    results["list"].append(VDMS_VID_PATH_PROP);
  }

  query.QueryNode(get_value<int>(cmd, "_ref", -1), VDMS_VID_TAG, cmd["link"],
                  cmd["constraints"], results,
                  get_value<bool>(cmd, "unique", false));

  return 0;
}

Json::Value FindVideo::construct_responses(Json::Value &responses,
                                           const Json::Value &json,
                                           protobufs::queryMessage &query_res,
                                           const std::string &blob) {
  const Json::Value &cmd = json[_cmd_name];

  Json::Value ret;

  auto error = [&](Json::Value &res) {
    ret[_cmd_name] = res;
    return ret;
  };

  Json::Value resp = check_responses(responses);
  if (resp["status"] != RSCommand::Success) {
    return error(resp);
  }

  Json::Value &FindVideo = responses[0];

  bool flag_empty = true;

  for (auto &ent : FindVideo["entities"]) {

    if (!ent.isMember(VDMS_VID_PATH_PROP)) {
      continue;
    }

    std::string video_path = ent[VDMS_VID_PATH_PROP].asString();
    ent.removeMember(VDMS_VID_PATH_PROP);

    if (ent.getMemberNames().size() > 0) {
      flag_empty = false;
    }
    try {
      if (!cmd.isMember("operations") && !cmd.isMember("container") &&
          !cmd.isMember("codec")) {
        // grab the video from aws and put it where vdms expects it
        if (_use_aws_storage) {
          VCL::RemoteConnection *connection = new VCL::RemoteConnection();
          std::string bucket = VDMSConfig::instance()->get_bucket_name();
          connection->_bucket_name = bucket;
          VCL::Video video(video_path);
          video.set_connection(connection);
          video._remote->Read_Video(
              video_path); // this takes the file from aws and puts it back in
                           // the local database location
        }

        // Return video as is.
        std::ifstream ifile(video_path, std::ifstream::in);
        ifile.seekg(0, std::ios::end);
        size_t encoded_size = (long)ifile.tellg();
        ifile.seekg(0, std::ios::beg);

        std::string *video_str = query_res.add_blobs();
        video_str->resize(encoded_size);
        ifile.read((char *)(video_str->data()), encoded_size);
        ifile.close();

        if (_use_aws_storage) {
          bool result = fs::remove(video_path);
        }
      } else {
        VCL::Video video(video_path);

        if (cmd.isMember("operations")) {
          enqueue_operations(video, cmd["operations"]);
        }

        const std::string &container =
            get_value<std::string>(cmd, "container", "mp4");
        const std::string &file_name =
            VCL::create_unique("/tmp/tmp/", container);
        const std::string &codec = get_value<std::string>(cmd, "codec", "h264");

        VCL::Video::Codec vcl_codec = string_to_codec(codec);
        video.store(file_name, vcl_codec); // to /tmp/ for encoding.

        auto video_enc = video.get_encoded();
        int size = video_enc.size();

        if (size > 0) {

          std::string *video_str = query_res.add_blobs();
          video_str->resize(size);
          std::memcpy((void *)video_str->data(), (void *)video_enc.data(),
                      size);
        } else {
          Json::Value return_error;
          return_error["status"] = RSCommand::Error;
          return_error["info"] = "Video Data not found";
          error(return_error);
        }
      }
    } catch (VCL::Exception e) {
      print_exception(e);
      Json::Value return_error;
      return_error["status"] = RSCommand::Error;
      return_error["info"] = "VCL Exception";
      return error(return_error);
    }
  }

  if (flag_empty) {
    FindVideo.removeMember("entities");
  }

  ret[_cmd_name].swap(FindVideo);
  return ret;
}

//========= FindFrames definitions =========

FindFrames::FindFrames() : VideoCommand("FindFrames") {
  //_use_aws_storage = VDMSConfig::instance()->get_aws_flag();
}

bool FindFrames::get_interval_index(const Json::Value &cmd,
                                    Json::ArrayIndex &op_index) {
  if (cmd.isMember("operations")) {
    const auto operations = cmd["operations"];
    for (auto i = 0; i < operations.size(); i++) {
      const auto op = operations[i];
      const std::string &type = get_value<std::string>(op, "type");
      if (type == "interval") {
        op_index = i;
        return true;
      }
    }
  }
  return false;
}

int FindFrames::construct_protobuf(PMGDQuery &query, const Json::Value &jsoncmd,
                                   const std::string &blob, int grp_id,
                                   Json::Value &error) {
  const Json::Value &cmd = jsoncmd[_cmd_name];

  // We try to catch the missing attribute error before
  // initiating a PMGD query
  Json::ArrayIndex tmp;
  bool is_interval = get_interval_index(cmd, tmp);
  bool is_frames = cmd.isMember("frames");

  if (!(is_frames != is_interval)) {
    error["status"] = RSCommand::Error;
    error["info"] = "Either one of 'frames' or 'operations::interval' "
                    "must be specified";
    return -1;
  }

  Json::Value results = get_value<Json::Value>(cmd, "results");
  results["list"].append(VDMS_VID_PATH_PROP);

  query.QueryNode(get_value<int>(cmd, "_ref", -1), VDMS_VID_TAG, cmd["link"],
                  cmd["constraints"], results,
                  get_value<bool>(cmd, "unique", false));

  return 0;
}

Json::Value FindFrames::construct_responses(Json::Value &responses,
                                            const Json::Value &json,
                                            protobufs::queryMessage &query_res,
                                            const std::string &blob) {
  const Json::Value &cmd = json[_cmd_name];

  Json::Value ret;

  auto error = [&](Json::Value &res) {
    ret[_cmd_name] = res;
    return ret;
  };

  Json::Value resp = check_responses(responses);
  if (resp["status"] != RSCommand::Success) {
    return error(resp);
  }

  Json::Value &FindFrames = responses[0];

  bool flag_empty = true;

  for (auto &ent : FindFrames["entities"]) {

    std::string video_path = ent[VDMS_VID_PATH_PROP].asString();
    ent.removeMember(VDMS_VID_PATH_PROP);

    if (ent.getMemberNames().size() > 0) {
      flag_empty = false;
    }

    try {
      std::vector<unsigned> frames;

      // Copy of operations is needed, as we pass the operations to
      // the enqueue_operations() method of ImageCommands class, and
      // it should not include 'interval' operation.
      Json::Value operations = cmd["operations"];

      Json::ArrayIndex interval_idx;
      bool is_interval = get_interval_index(cmd, interval_idx);
      bool is_frames = cmd.isMember("frames");

      if (is_frames) {
        for (auto &fr : cmd["frames"]) {
          frames.push_back(fr.asUInt());
        }
      } else if (is_interval) {

        Json::Value interval_op = operations[interval_idx];

        int start = get_value<int>(interval_op, "start");
        int stop = get_value<int>(interval_op, "stop");
        int step = get_value<int>(interval_op, "step");

        for (int i = start; i < stop; i += step) {
          frames.push_back(i);
        }

        Json::Value deleted;
        operations.removeIndex(interval_idx, &deleted);
      } else {
        // This should never happen, as we check this condition in
        // FindFrames::construct_protobuf(). In case this happens, it
        // is better to signal it rather than to continue
        Json::Value return_error;
        return_error["status"] = RSCommand::Error;
        return_error["info"] = "No 'frames' or 'interval' parameter";
        return error(return_error);
      }

      VCL::Video video(video_path);

      // grab the video from aws here if necessary
      if (_use_aws_storage) {
        VCL::RemoteConnection *connection = new VCL::RemoteConnection();
        std::string bucket = VDMSConfig::instance()->get_bucket_name();
        connection->_bucket_name = bucket;
        VCL::Video video(video_path);
        video.set_connection(connection);
        video._remote->Read_Video(
            video_path); // this takes the file from aws and puts it back in the
                         // local database location
      }

      // By default, return frames as PNGs
      VCL::Image::Format format = VCL::Image::Format::PNG;

      FindImage img_cmd;

      if (cmd.isMember("format")) {

        format = img_cmd.get_requested_format(cmd);

        if (format == VCL::Image::Format::NONE_IMAGE ||
            format == VCL::Image::Format::TDB) {
          Json::Value return_error;
          return_error["status"] = RSCommand::Error;
          return_error["info"] = "Invalid Return Format for FindFrames";
          return error(return_error);
        }
      }

      for (auto idx : frames) {
        cv::Mat mat = video.get_frame(idx);
        VCL::Image img(mat, false);
        if (!operations.empty()) {
          img_cmd.enqueue_operations(img, operations);
        }

        std::vector<unsigned char> img_enc;
        img_enc = img.get_encoded_image(format);

        if (!img_enc.empty()) {
          std::string *img_str = query_res.add_blobs();
          img_str->resize(img_enc.size());
          std::memcpy((void *)img_str->data(), (void *)img_enc.data(),
                      img_enc.size());
        } else {
          Json::Value return_error;
          return_error["status"] = RSCommand::Error;
          return_error["info"] = "Image Data not found";
          return error(return_error);
        }
      }

      // delete the video from local storage here, done with it for now
      if (_use_aws_storage) {
        std::remove(video_path.c_str());
      }
    }

    catch (VCL::Exception e) {
      print_exception(e);
      Json::Value return_error;
      return_error["status"] = RSCommand::Error;
      return_error["info"] = "VCL Exception";
      return error(return_error);
    }
  }

  if (flag_empty) {
    FindFrames.removeMember("entities");
  }

  ret[_cmd_name].swap(FindFrames);
  return ret;
}
