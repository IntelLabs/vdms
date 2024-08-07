/**
 * @file   ImageCommand.cc
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2023 Intel Corporation
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

#include <iostream>

#include "ImageCommand.h"
#include "VDMSConfig.h"
#include "defines.h"

#include "ImageLoop.h"

using namespace VDMS;

//========= AddImage definitions =========

ImageCommand::ImageCommand(const std::string &cmd_name) : RSCommand(cmd_name) {
  output_vcl_timing =
      VDMSConfig::instance()->get_bool_value("print_vcl_timing", false);
}

int ImageCommand::enqueue_operations(VCL::Image &img, const Json::Value &ops,
                                     bool is_addition) {
  // Correct operation type and parameters are guaranteed at this point
  for (auto &op : ops) {
    const std::string &type = get_value<std::string>(op, "type");
    if (type == "threshold") {
      img.threshold(get_value<int>(op, "value"));
    } else if (type == "resize") {
      img.resize(get_value<int>(op, "height"), get_value<int>(op, "width"));
    } else if (type == "crop") {
      img.crop(VCL::Rectangle(get_value<int>(op, "x"), get_value<int>(op, "y"),
                              get_value<int>(op, "width"),
                              get_value<int>(op, "height")));
    } else if (type == "flip") {
      img.flip(get_value<int>(op, "code"));
    } else if (type == "rotate") {
      img.rotate(get_value<double>(op, "angle"), get_value<bool>(op, "resize"));
    } else if (type == "syncremoteOp") {
      Json::Value options = get_value<Json::Value>(op, "options");
      if (is_addition) {
        options["ingestion"] = 1;
      }
      img.syncremoteOperation(get_value<std::string>(op, "url"), options);
    } else if (type == "remoteOp") {
      Json::Value options = get_value<Json::Value>(op, "options");
      if (is_addition) {
        options["ingestion"] = 1;
        img.syncremoteOperation(get_value<std::string>(op, "url"), options);
      } else {
        img.remoteOperation(get_value<std::string>(op, "url"), options);
      }
    } else if (type == "userOp") {
      img.userOperation(get_value<Json::Value>(op, "options"));
    } else if (type == "custom") {
      VCL::Image *tmp_image = new VCL::Image(img, true);
      try {
        if (custom_vcl_function(img, op) != 0) {
          img.deep_copy_cv(tmp_image->get_cvmat(
              true)); // function completed but error detected
          delete tmp_image;
          return -1;
        }
      } catch (...) {
        img.deep_copy_cv(
            tmp_image->get_cvmat(true)); // function threw exception
        delete tmp_image;
        return -1;
      }
      delete tmp_image;
    } else {
      throw ExceptionCommand(ImageError, "Operation not defined");
      return -1;
    }
  }
  return 0;
}

VCL::Format ImageCommand::get_requested_format(const Json::Value &cmd) {
  VCL::Format format;

  std::string requested_format = get_value<std::string>(cmd, "format", "");

  if (requested_format == "png") {
    return VCL::Format::PNG;
  }
  if (requested_format == "jpg") {
    return VCL::Format::JPG;
  }
  if (requested_format == "tdb") {
    return VCL::Format::TDB;
  }
  if (requested_format == "bin") {
    return VCL::Format::BIN;
  }

  return VCL::Format::NONE_IMAGE;
}

//========= AddImage definitions =========

AddImage::AddImage() : ImageCommand("AddImage") {
  _storage_tdb = VDMSConfig::instance()->get_path_tdb();
  _storage_png = VDMSConfig::instance()->get_path_png();
  _storage_jpg = VDMSConfig::instance()->get_path_jpg();
  _storage_bin = VDMSConfig::instance()->get_path_bin();
  _use_aws_storage = VDMSConfig::instance()->get_aws_flag();
}

int AddImage::construct_protobuf(PMGDQuery &query, const Json::Value &jsoncmd,
                                 const std::string &blob, int grp_id,
                                 Json::Value &error) {

  const Json::Value &cmd = jsoncmd[_cmd_name];
  int operation_flags = 0;
  bool sameFormat = false;
  int node_ref = get_value<int>(cmd, "_ref", query.get_available_reference());
  const std::string from_file_path =
      get_value<std::string>(cmd, "from_file_path", "");
  const bool is_local_file = get_value<bool>(cmd, "is_local_file", false);
  std::string format = get_value<std::string>(cmd, "format", "");
  char binary_img_flag = 0;
  if (format == "bin") {
    binary_img_flag = 1;
  }

  std::string img_root = _storage_tdb;
  std::string file_name;
  VCL::Format input_format =
      VCL::read_image_format((void *)blob.data(), blob.size());
  std::string image_fomrat = VCL::format_to_string(input_format);
  if ((image_fomrat == format) && (!cmd.isMember("operations"))) {
    if (image_fomrat == "png")
      img_root = _storage_png;
    else if (image_fomrat == "jpg")
      img_root = _storage_jpg;
    file_name = VCL::create_unique(img_root, format);
    Json::Value props = get_value<Json::Value>(cmd, "properties");
    props[VDMS_IM_PATH_PROP] = file_name;

    query.AddNode(node_ref, VDMS_IM_TAG, props, Json::Value());
    VCL::Image img;

    if (from_file_path.empty()) {
      img = VCL::Image(file_name, blob, input_format);
    } else {
      if (is_local_file) {
        img = VCL::Image(from_file_path, false);
      } else {
        img = VCL::Image(from_file_path, true);
      }
    }

    if (_use_aws_storage) {
      VCL::RemoteConnection *connection = new VCL::RemoteConnection();
      std::string bucket = VDMSConfig::instance()->get_bucket_name();
      connection->_bucket_name = bucket;
      img.set_connection(connection);
    }
    img.save_image(file_name, blob);

  } else { // used when input format is not the same as the output format
    VCL::Image img;

    if (from_file_path.empty()) {
      img = VCL::Image((void *)blob.data(), blob.size(), binary_img_flag);
    } else {
      if (is_local_file) {
        img = VCL::Image(from_file_path, false);
      } else {
        img = VCL::Image(from_file_path, true);
      }
    }

    if (_use_aws_storage) {
      VCL::RemoteConnection *connection = new VCL::RemoteConnection();
      std::string bucket = VDMSConfig::instance()->get_bucket_name();
      connection->_bucket_name = bucket;
      img.set_connection(connection);
    }
    if (cmd.isMember("operations")) {
      operation_flags = enqueue_operations(img, cmd["operations"], true);
    }

    if (operation_flags != 0) {
      error["info"] = "custom function process not found";
      error["status"] = RSCommand::Error;
      return -1;
    }

    if (format == "png") {
      input_format = VCL::Format::PNG;
      img_root = _storage_png;
    } else if (format == "tdb") {
      input_format = VCL::Format::TDB;
      img_root = _storage_tdb;
    } else if (format == "jpg") {
      input_format = VCL::Format::JPG;
      img_root = _storage_jpg;
    } else if (format == "bin") {
      input_format = VCL::Format::BIN;
      img_root = _storage_bin;
    } else {
      error["info"] = format + ": format not implemented";
      error["status"] = RSCommand::Error;
      return -1;
    }

    file_name = VCL::create_unique(img_root, format);

    // Modifiyng the existing properties that the user gives
    // is a good option to make the AddNode more simple.
    // This is not ideal since we are manupulating with user's
    // input, but for now it is an acceptable solution.
    Json::Value props = get_value<Json::Value>(cmd, "properties");
    props[VDMS_IM_PATH_PROP] = file_name;

    if (img.is_blob_not_stored()) {
      props[VDMS_IM_PATH_PROP] = from_file_path;
      file_name = from_file_path;
    }
    // Add Image node
    query.AddNode(node_ref, VDMS_IM_TAG, props, Json::Value());

    img.store(file_name, input_format);

    std::vector<Json::Value> image_metadata = img.get_ingest_metadata();

    if (image_metadata.size() > 0) {
      for (Json::Value metadata : image_metadata) {
        int bb_ref = query.get_available_reference();
        Json::Value bbox_props;
        bbox_props[VDMS_DM_IMG_NAME_PROP] = props[VDMS_IM_PATH_PROP];
        bbox_props[VDMS_DM_IMG_OBJECT_PROP] = metadata["object"].asString();
        bbox_props[VDMS_ROI_COORD_X_PROP] = metadata["x"].asFloat();
        bbox_props[VDMS_ROI_COORD_Y_PROP] = metadata["y"].asFloat();
        bbox_props[VDMS_ROI_WIDTH_PROP] = metadata["width"].asFloat();
        bbox_props[VDMS_ROI_HEIGHT_PROP] = metadata["height"].asFloat();
        bbox_props[VDMS_DM_VID_OBJECT_DET] =
            metadata["object_det"].toStyledString();

        Json::Value bb_edge_props;
        bb_edge_props[VDMS_DM_IMG_NAME_PROP] = props[VDMS_IM_PATH_PROP];

        query.AddNode(bb_ref, VDMS_ROI_TAG, bbox_props, Json::Value());
        query.AddEdge(-1, node_ref, bb_ref, VDMS_DM_IMG_BB_EDGE, bb_edge_props);
      }
    }

    if (output_vcl_timing) {
      img.timers.print_map_runtimes();
    }
  }

  // In case we need to cleanup the query
  error["image_added"] = file_name;

  if (cmd.isMember("link")) {
    add_link(query, cmd["link"], node_ref, VDMS_IM_EDGE_TAG);
  }

  return 0;
}

bool AddImage::need_blob(const Json::Value &cmd) {
  if (cmd.isMember(_cmd_name)) {
    const Json::Value &add_image_cmd = cmd[_cmd_name];
    return !(add_image_cmd.isMember("from_file_path"));
  }
  throw VCLException(UndefinedException, "Query Error");
}

//========= UpdateImage definitions =========

UpdateImage::UpdateImage() : ImageCommand("UpdateImage") {}

int UpdateImage::construct_protobuf(PMGDQuery &query,
                                    const Json::Value &jsoncmd,
                                    const std::string &blob, int grp_id,
                                    Json::Value &error) {
  const Json::Value &cmd = jsoncmd[_cmd_name];

  // Update Image node
  query.UpdateNode(get_value<int>(cmd, "_ref", -1), VDMS_IM_TAG,
                   cmd["properties"], cmd["remove_props"], cmd["constraints"],
                   get_value<bool>(cmd, "unique", false));

  return 0;
}

//========= FindImage definitions =========

FindImage::FindImage() : ImageCommand("FindImage") {
  _use_aws_storage = VDMSConfig::instance()->get_aws_flag();
}

int FindImage::construct_protobuf(PMGDQuery &query, const Json::Value &jsoncmd,
                                  const std::string &blob, int grp_id,
                                  Json::Value &error) {
  const Json::Value &cmd = jsoncmd[_cmd_name];

  Json::Value results = get_value<Json::Value>(cmd, "results");

  // Unless otherwise specified, we return the blob.
  if (get_value<bool>(results, "blob", true)) {
    results["list"].append(VDMS_IM_PATH_PROP);
  }

  if (cmd.isMember("metaconstraints")) {
    results["list"].append(VDMS_DM_IMG_NAME_PROP);

    for (auto member : cmd["metaconstraints"].getMemberNames()) {
      results["list"].append(member);
    }

    results["list"].append(VDMS_DM_IMG_OBJECT_PROP);
    results["list"].append(VDMS_DM_IMG_OBJECT_DET);
    results["list"].append(VDMS_ROI_COORD_X_PROP);
    results["list"].append(VDMS_ROI_COORD_Y_PROP);
    results["list"].append(VDMS_ROI_WIDTH_PROP);
    results["list"].append(VDMS_ROI_HEIGHT_PROP);

    query.QueryNode(get_value<int>(cmd, "_ref", -1), VDMS_ROI_TAG, cmd["link"],
                    cmd["metaconstraints"], results,
                    get_value<bool>(cmd, "unique", false));
  } else {
    query.QueryNode(get_value<int>(cmd, "_ref", -1), VDMS_IM_TAG, cmd["link"],
                    cmd["constraints"], results,
                    get_value<bool>(cmd, "unique", false));
  }

  return 0;
}

Json::Value FindImage::construct_responses(Json::Value &responses,
                                           const Json::Value &json,
                                           protobufs::queryMessage &query_res,
                                           const std::string &blob) {
  const Json::Value &cmd = json[_cmd_name];
  int operation_flags = 0;
  bool has_operations = false;
  std::string no_op_def_image;
  Json::Value ret;

  std::map<std::string, VCL::Format> formats;

  auto error = [&](Json::Value &res) {
    ret[_cmd_name] = res;
    return ret;
  };

  auto empty = [&](Json::Value &res) {
    ret[_cmd_name] = res;
    return ret;
  };

  if (responses.size() != 1) {
    Json::Value return_error;
    return_error["status"] = RSCommand::Error;
    return_error["info"] = "PMGD Response Bad Size";
    return error(return_error);
  }

  Json::Value &findImage = responses[0];

  if (findImage["status"] != 0) {
    findImage["status"] = RSCommand::Error;
    // Uses PMGD info error.
    return error(findImage);
  }
  Json::Value results = get_value<Json::Value>(cmd, "results");

  bool flag_empty = false;

  if (findImage["entities"].size() == 0) {
    Json::Value return_empty;
    return_empty["status"] = RSCommand::Success;
    return_empty["info"] = "No entities found";
    return empty(return_empty);
  }

  // Check if blob (image) must be returned
  if (get_value<bool>(results, "blob", true)) {

    ImageLoop eventloop;
    eventloop.set_nrof_entities(findImage["entities"].size());

    for (auto &ent : findImage["entities"]) {
      assert(ent.isMember(VDMS_IM_PATH_PROP));

      std::string im_path;
      if (cmd.isMember("metaconstraints")) {
        im_path = ent[VDMS_DM_IMG_NAME_PROP].asString();
      } else {
        im_path = ent[VDMS_IM_PATH_PROP].asString();
        ent.removeMember(VDMS_IM_PATH_PROP);
      }

      if (ent.getMemberNames().size() == 0) {
        flag_empty = true;
      }

      try {
        VCL::Image img(im_path);
        if (_use_aws_storage) {
          VCL::RemoteConnection *connection = new VCL::RemoteConnection();
          std::string bucket = VDMSConfig::instance()->get_bucket_name();
          connection->_bucket_name = bucket;
          img.set_connection(connection);
        }

        if (cmd.isMember("operations")) {
          operation_flags = enqueue_operations(img, cmd["operations"]);
          has_operations = true;
        }

        // We will return the image in the format the user
        // request, or on its format in disk, except for the case
        // of .tdb, where we will encode as png.
        VCL::Format format = img.get_image_format() != VCL::Format::TDB
                                 ? img.get_image_format()
                                 : VCL::Format::PNG;

        if (operation_flags != 0) {
          Json::Value return_error;
          return_error["info"] = "custom function process not found";
          return_error["status"] = RSCommand::Error;
          return error(return_error);
        }
        if (cmd.isMember("format")) {
          format = get_requested_format(cmd);
          if (format == VCL::Format::NONE_IMAGE || format == VCL::Format::TDB) {
            Json::Value return_error;
            return_error["status"] = RSCommand::Error;
            return_error["info"] = "Invalid Requested Format for FindImage";
            return error(return_error);
          }
        }

        if (has_operations) {
          formats.insert(
              std::pair<std::string, VCL::Format>(img.get_image_id(), format));
          eventloop.enqueue(&img);
        } else {
          std::vector<unsigned char> img_enc;
          img_enc = img.get_encoded_image(format);
          no_op_def_image = img.get_image_id();
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

      } catch (VCL::Exception e) {
        print_exception(e);
        Json::Value return_error;
        return_error["status"] = RSCommand::Error;
        return_error["info"] = "VCL Exception";
        return error(return_error);
      }
    }

    if (has_operations) {
      while (eventloop.is_loop_running()) {
        continue;
      }
      std::map<std::string, VCL::Image *> imageMap = eventloop.get_image_map();
      std::map<std::string, VCL::Image *>::iterator iter = imageMap.begin();

      if (iter->second->get_query_error_response() != "") {
        Json::Value return_error;
        return_error["status"] = RSCommand::Error;
        return_error["info"] = iter->second->get_query_error_response();
        return error(return_error);
      }

      while (iter != imageMap.end()) {
        std::vector<unsigned char> img_enc =
            iter->second->get_encoded_image_async(formats[iter->first]);
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

        if (output_vcl_timing) {
          iter->second->timers.print_map_runtimes();
        }

        iter++;
      }
    } else {
      eventloop.close_no_operation_loop(no_op_def_image);
    }
  }
  if (flag_empty) {
    findImage.removeMember("entities");
  }
  ret[_cmd_name].swap(findImage);
  return ret;
}
