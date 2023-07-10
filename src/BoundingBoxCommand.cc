/**
 * @file   BoundingBoxCommand.cc
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

#include <iostream>

#include "BoundingBoxCommand.h"
#include "VDMSConfig.h"
#include "defines.h"
#include "vcl/Image.h"

using namespace VDMS;

//========= AddBoundingBox definitions =========

AddBoundingBox::AddBoundingBox() : RSCommand("AddBoundingBox") {}

int AddBoundingBox::construct_protobuf(PMGDQuery &query,
                                       const Json::Value &jsoncmd,
                                       const std::string &blob, int grp_id,
                                       Json::Value &error) {
  const Json::Value &cmd = jsoncmd[_cmd_name];

  int node_ref = get_value<int>(cmd, "_ref", query.get_available_reference());

  Json::Value rect = get_value<Json::Value>(cmd, "rectangle");
  Json::Value props = get_value<Json::Value>(cmd, "properties");

  props[VDMS_ROI_COORD_X_PROP] = get_value<int>(rect, "x");
  props[VDMS_ROI_COORD_Y_PROP] = get_value<int>(rect, "y");
  props[VDMS_ROI_WIDTH_PROP] = get_value<int>(rect, "w");
  props[VDMS_ROI_HEIGHT_PROP] = get_value<int>(rect, "h");

  // Add Region node
  query.AddNode(node_ref, VDMS_ROI_TAG, props, Json::Value());

  if (cmd.isMember("image")) {
    Json::Value img;
    img["ref"] = cmd["image"];
    add_link(query, img, node_ref, VDMS_ROI_IMAGE_EDGE);
  }

  if (cmd.isMember("link")) {
    Json::Value ent;
    ent = cmd["link"];
    add_link(query, ent, node_ref, VDMS_ROI_EDGE_TAG);
  }

  return 0;
}

//========= UpdateBoundingBox definitions =========

UpdateBoundingBox::UpdateBoundingBox() : RSCommand("UpdateBoundingBox") {}

int UpdateBoundingBox::construct_protobuf(PMGDQuery &query,
                                          const Json::Value &jsoncmd,
                                          const std::string &blob, int grp_id,
                                          Json::Value &error) {
  const Json::Value &cmd = jsoncmd[_cmd_name];

  Json::Value rect =
      get_value<Json::Value>(cmd, "rectangle", Json::Value::null);
  Json::Value props = get_value<Json::Value>(cmd, "properties");

  if (rect != Json::Value::null) {
    props[VDMS_ROI_COORD_X_PROP] = get_value<int>(rect, "x");
    props[VDMS_ROI_COORD_Y_PROP] = get_value<int>(rect, "y");
    props[VDMS_ROI_WIDTH_PROP] = get_value<int>(rect, "w");
    props[VDMS_ROI_HEIGHT_PROP] = get_value<int>(rect, "h");
  }

  // Update Bounding Box
  query.UpdateNode(get_value<int>(cmd, "_ref", -1), VDMS_ROI_TAG, props,
                   cmd["remove_props"], cmd["constraints"],
                   get_value<bool>(cmd, "unique", false));

  return 0;
}

//========= FindBoundingBox definitions =========

FindBoundingBox::FindBoundingBox() : RSCommand("FindBoundingBox") {
  //_use_aws_storage = VDMSConfig::instance()->get_aws_flag();
}

int FindBoundingBox::construct_protobuf(PMGDQuery &query,
                                        const Json::Value &jsoncmd,
                                        const std::string &blob, int grp_id,
                                        Json::Value &error) {
  const Json::Value &cmd = jsoncmd[_cmd_name];

  // if blob is true, make sure we have a reference, that way we can iterate
  // over the bounding boxes and find the link to the image (if it exists)
  Json::Value results = get_value<Json::Value>(cmd, "results");
  int ref = get_value<int>(cmd, "_ref", query.get_available_reference());

  bool coords = false;
  if (results.isMember("list")) {
    for (int i = 0; i < results["list"].size(); ++i) {
      if (results["list"][i].asString() == "_coordinates") {
        Json::Value aux;
        results["list"].removeIndex(i, &aux);
        coords = true;
        break;
      }
    }
  }

  if (get_value<bool>(results, "blob"))
    coords = true;

  if (coords) {
    results["list"].append(VDMS_ROI_COORD_X_PROP);
    results["list"].append(VDMS_ROI_COORD_Y_PROP);
    results["list"].append(VDMS_ROI_WIDTH_PROP);
    results["list"].append(VDMS_ROI_HEIGHT_PROP);
  }

  Json::Value link;
  if (cmd.isMember("image")) {
    link["ref"] = get_value<int>(cmd, "image", -1);
    link["class"] = VDMS_ROI_IMAGE_EDGE;
  } else
    link = cmd["link"];

  Json::Value constraints = cmd["constraints"];
  if (cmd.isMember("rectangle")) {
    Json::Value rect = get_value<Json::Value>(cmd, "rectangle", -1);
    constraints[VDMS_ROI_COORD_X_PROP].append(">=");
    constraints[VDMS_ROI_COORD_X_PROP].append(get_value<int>(rect, "x"));
    constraints[VDMS_ROI_COORD_X_PROP].append("<=");
    constraints[VDMS_ROI_COORD_X_PROP].append(get_value<int>(rect, "w"));

    constraints[VDMS_ROI_COORD_Y_PROP].append(">=");
    constraints[VDMS_ROI_COORD_Y_PROP].append(get_value<int>(rect, "y"));
    constraints[VDMS_ROI_COORD_Y_PROP].append("<=");
    constraints[VDMS_ROI_COORD_Y_PROP].append(get_value<int>(rect, "h"));

    constraints[VDMS_ROI_WIDTH_PROP].append("<=");
    constraints[VDMS_ROI_WIDTH_PROP].append(get_value<int>(rect, "w") +
                                            get_value<int>(rect, "x"));

    constraints[VDMS_ROI_HEIGHT_PROP].append("<=");
    constraints[VDMS_ROI_HEIGHT_PROP].append(get_value<int>(rect, "h") +
                                             get_value<int>(rect, "y"));
  }

  query.QueryNode(ref, VDMS_ROI_TAG, link, constraints, results,
                  get_value<bool>(cmd, "unique", false));

  if (get_value<bool>(results, "blob", false)) {
    Json::Value imgresults;
    imgresults["list"].append(VDMS_IM_PATH_PROP);

    Json::Value imglink;
    imglink["ref"] = ref;

    query.QueryNode(-1, VDMS_IM_TAG, imglink, Json::Value::null, imgresults,
                    get_value<bool>(cmd, "unique", false));
  }

  return 0;
}

Json::Value FindBoundingBox::construct_responses(
    Json::Value &responses, const Json::Value &json,
    protobufs::queryMessage &query_res, const std::string &blob) {
  const Json::Value &cmd = json[_cmd_name];
  const Json::Value &results = cmd["results"];

  Json::Value ret;

  auto error = [&](Json::Value &res) {
    ret[_cmd_name] = res;
    return ret;
  };

  if (responses.size() == 0) {
    Json::Value return_error;
    return_error["status"] = RSCommand::Error;
    return_error["info"] = "PMGD Found Nothing when Looking for BoundingBoxes";
    return error(return_error);
  }

  Json::Value &findBB = responses[0];

  if (findBB["status"] != 0) {
    Json::Value return_error;
    return_error["status"] = RSCommand::Error;
    return_error["info"] = "BoundingBox Not Found";
    return error(return_error);
  }

  Json::Value entities = findBB["entities"];
  findBB.removeMember("entities");

  for (int i = 0; i < entities.size(); ++i) {
    auto ent = entities[i];
    Json::Value bb;

    Json::Value coords;
    if (ent.isMember(VDMS_ROI_COORD_X_PROP) &&
        ent.isMember(VDMS_ROI_COORD_Y_PROP) &&
        ent.isMember(VDMS_ROI_WIDTH_PROP) &&
        ent.isMember(VDMS_ROI_HEIGHT_PROP)) {
      coords["x"] = ent[VDMS_ROI_COORD_X_PROP];
      coords["y"] = ent[VDMS_ROI_COORD_Y_PROP];
      coords["w"] = ent[VDMS_ROI_WIDTH_PROP];
      coords["h"] = ent[VDMS_ROI_HEIGHT_PROP];
    }

    if (results.isMember("list")) {
      for (int i = 0; i < results["list"].size(); ++i) {
        auto current = results["list"][i].asString();
        if (current == "_coordinates") {
          bb["_coordinates"] = coords;
        } else {
          bb[current] = ent[current];
        }
      }
    }

    if (get_value<bool>(results, "blob", false)) {
      if (responses.size() < 1) {
        Json::Value return_error;
        return_error["status"] = RSCommand::Error;
        return_error["info"] = "BoundingBox is Missing Corresponding Blob";
        return error(return_error);
      }

      Json::Value &findImage = responses[1];
      if (findImage["status"] != 0) {
        findImage["status"] = RSCommand::Error;
        // Uses PMGD info error.
        error(findImage);
      }
      if (findImage["entities"].size() <= i)
        continue;
      else {
        bool flag_empty = true;

        auto img_ent = findImage["entities"][i];

        assert(img_ent.isMember(VDMS_IM_PATH_PROP));
        std::string im_path = img_ent[VDMS_IM_PATH_PROP].asString();
        img_ent.removeMember(VDMS_IM_PATH_PROP);

        if (img_ent.getMemberNames().size() > 0) {
          flag_empty = false;
        }

        try {
          std::string bucket_name = "";
          if (_use_aws_storage) {
            bucket_name = VDMSConfig::instance()->get_bucket_name();
          }

          VCL::Image img(im_path, bucket_name);

          img.crop(VCL::Rectangle(
              get_value<int>(coords, "x"), get_value<int>(coords, "y"),
              get_value<int>(coords, "w"), get_value<int>(coords, "h")));

          VCL::Image::Format format =
              img.get_image_format() != VCL::Image::Format::TDB
                  ? img.get_image_format()
                  : VCL::Image::Format::PNG;

          if (cmd.isMember("format")) {
            std::string requested_format =
                get_value<std::string>(cmd, "format");

            if (requested_format == "png") {
              format = VCL::Image::Format::PNG;
            } else if (requested_format == "jpg") {
              format = VCL::Image::Format::JPG;
            }
          }

          std::vector<unsigned char> roi_enc;
          roi_enc = img.get_encoded_image(format);

          if (!roi_enc.empty()) {
            std::string *img_str = query_res.add_blobs();
            img_str->resize(roi_enc.size());
            std::memcpy((void *)img_str->data(), (void *)roi_enc.data(),
                        roi_enc.size());
          } else {
            Json::Value return_error;
            return_error["status"] = RSCommand::Error;
            return_error["info"] = "Image Data not found";
            error(return_error);
          }
        } catch (VCL::Exception e) {
          print_exception(e);
          Json::Value return_error;
          return_error["status"] = RSCommand::Error;
          return_error["info"] = "VCL Exception";
          error(return_error);
        }

        if (!flag_empty) {
          findImage.removeMember("entities");
        }
      }
    }

    findBB["entities"].append(bb);
  }

  findBB["status"] = RSCommand::Success;
  ret[_cmd_name] = findBB;

  return ret;
}
