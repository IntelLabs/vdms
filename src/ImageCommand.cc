/**
 * @file   ImageCommand.cc
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

#include "ImageCommand.h"
#include "VDMSConfig.h"

using namespace VDMS;

#define VDMS_IM_TAG           "VDMS:IMG"
#define VDMS_IM_EDGE          "VDMS:IMGLINK"
#define VDMS_IM_PATH_PROP     "VDMS:imgPath"

ImageCommand::ImageCommand(const std::string &cmd_name):
    RSCommand(cmd_name)
{
}

void ImageCommand::enqueue_operations(VCL::Image& img, const Json::Value& ops)
{
    // Correct operation type and parameters are guaranteed at this point
    for (auto& op : ops) {
        const std::string& type = get_value<std::string>(op, "type");
        if (type == "threshold") {
            img.threshold(get_value<int>(op, "value"));
        }
        else if (type == "resize") {
            img.resize(get_value<int>(op, "height"),
                          get_value<int>(op, "width") );
        }
        else if (type == "crop") {
            img.crop(VCL::Rectangle (
                        get_value<int>(op, "x"),
                        get_value<int>(op, "y"),
                        get_value<int>(op, "width"),
                        get_value<int>(op, "height") ));
        }
        else {
            throw ExceptionCommand(ImageError, "Operation not defined");
        }
    }
}

//========= AddImage definitions =========

AddImage::AddImage() : ImageCommand("AddImage")
{
    _storage_tdb = VDMSConfig::instance()
                ->get_string_value("tdb_path", DEFAULT_TDB_PATH);
    _storage_png = VDMSConfig::instance()
                ->get_string_value("png_path", DEFAULT_PNG_PATH);
    _storage_jpg = VDMSConfig::instance()
                ->get_string_value("jpg_path", DEFAULT_JPG_PATH);
}

int AddImage::construct_protobuf(PMGDQuery& query,
    const Json::Value& jsoncmd,
    const std::string& blob,
    int grp_id,
    Json::Value& error)
{
    const Json::Value& cmd = jsoncmd[_cmd_name];

    int node_ref = get_value<int>(cmd, "_ref",
                                  query.get_available_reference());

    VCL::Image img((void*)blob.data(), blob.size());

    if (cmd.isMember("operations")) {
        enqueue_operations(img, cmd["operations"]);
    }

    std::string img_root = _storage_tdb;
    VCL::ImageFormat vcl_format = VCL::TDB;

    if (cmd.isMember("format")) {
        std::string format = get_value<std::string>(cmd, "format");

        if (format == "png") {
            vcl_format = VCL::PNG;
            img_root = _storage_png;
        }
        else if (format == "tdb") {
            vcl_format = VCL::TDB;
            img_root = _storage_tdb;
        }
        else if (format == "jpg") {
            vcl_format = VCL::JPG;
            img_root = _storage_jpg;
        }
        else {
            error["info"] = format + ": format not implemented";
            error["status"] = RSCommand::Error;
            return -1;
        }
    }

    std::string file_name = img.create_unique(img_root, vcl_format);

    // Modifiyng the existing properties that the user gives
    // is a good option to make the AddNode more simple.
    // This is not ideal since we are manupulating with user's
    // input, but for now it is an acceptable solution.
    Json::Value props = get_value<Json::Value>(cmd, "properties");
    props[VDMS_IM_PATH_PROP] = file_name;

    // Add Image node
    query.AddNode(node_ref, VDMS_IM_TAG, props, Json::Value());

    img.store(file_name, vcl_format);

    // In case we need to cleanup the query
    error["image_added"] = file_name;

    if (cmd.isMember("link")) {
        add_link(query, cmd["link"], node_ref, VDMS_IM_EDGE);
    }

    return 0;
}

//========= UpdateImage definitions =========

UpdateImage::UpdateImage() : ImageCommand("UpdateImage")
{
}

int UpdateImage::construct_protobuf(PMGDQuery& query,
    const Json::Value& jsoncmd,
    const std::string& blob,
    int grp_id,
    Json::Value& error)
{
    const Json::Value& cmd = jsoncmd[_cmd_name];

    int node_ref = get_value<int>(cmd, "_ref", -1);

    Json::Value constraints = get_value<Json::Value>(cmd, "constraints");

    Json::Value props = get_value<Json::Value>(cmd, "properties");

    Json::Value remove_props = get_value<Json::Value>(cmd, "remove_props");

    // Update Image node
    query.UpdateNode(node_ref, VDMS_IM_TAG, props,
                        remove_props,
                        constraints,
                        get_value<bool>(cmd, "unique", false));

    return 0;
}

Json::Value UpdateImage::construct_responses(
    Json::Value& responses,
    const Json::Value& json,
    protobufs::queryMessage &query_res)
{
    assert(responses.size() == 1);

    Json::Value ret;

    // TODO In order to support "format" or "operations", we could
    // implement VCL save operation here.

    ret[_cmd_name].swap(responses[0]);
    return ret;
}

//========= FindImage definitions =========

FindImage::FindImage() : ImageCommand("FindImage")
{
}

int FindImage::construct_protobuf(
    PMGDQuery& query,
    const Json::Value& jsoncmd,
    const std::string& blob,
    int grp_id,
    Json::Value& error)
{
    const Json::Value& cmd = jsoncmd[_cmd_name];

    Json::Value results = get_value<Json::Value>(cmd, "results");

    // Unless otherwhis specified, we return the blob.
    if (get_value<bool>(results, "blob", true)){
        results["list"].append(VDMS_IM_PATH_PROP);
    }

    query.QueryNode(
            get_value<int>(cmd, "_ref", -1),
            VDMS_IM_TAG,
            cmd["link"],
            cmd["constraints"],
            results,
            get_value<bool>(cmd, "unique", false)
            );

    return 0;
}

Json::Value FindImage::construct_responses(
    Json::Value& responses,
    const Json::Value& json,
    protobufs::queryMessage &query_res)
{
    const Json::Value& cmd = json[_cmd_name];

    Json::Value ret;

    auto error = [&](Json::Value& res)
    {
        ret[_cmd_name] = res;
        return ret;
    };

    if (responses.size() != 1) {
        Json::Value return_error;
        return_error["status"]  = RSCommand::Error;
        return_error["info"] = "PMGD Response Bad Size";
        error(return_error);
    }

    Json::Value& findImage = responses[0];

    assert(findImage.isMember("entities"));

    if (findImage["status"] != 0) {
        findImage["status"]  = RSCommand::Error;
        // Uses PMGD info error.
        error(findImage);
    }

    bool flag_empty = true;

    for (auto& ent : findImage["entities"]) {

        if(!ent.isMember(VDMS_IM_PATH_PROP)){
            continue;
        }
        std::string im_path = ent[VDMS_IM_PATH_PROP].asString();
        ent.removeMember(VDMS_IM_PATH_PROP);

        if (ent.getMemberNames().size() > 0) {
            flag_empty = false;
        }

        try {
            VCL::Image img(im_path);

            if (cmd.isMember("operations")) {
                enqueue_operations(img, cmd["operations"]);
            }

            // We will return the image in the format the user
            // request, or on its format in disk, except for the case
            // of .tdb, where we will encode as png.
            VCL::ImageFormat format = img.get_image_format() != VCL::TDB ?
                                      img.get_image_format() : VCL::PNG;

            if (cmd.isMember("format")) {
                std::string requested_format =
                            get_value<std::string>(cmd, "format");

                if (requested_format == "png") {
                    format = VCL::PNG;
                }
                else if (requested_format == "jpg") {
                    format = VCL::JPG;
                }
                else {
                    Json::Value return_error;
                    return_error["status"]  = RSCommand::Error;
                    return_error["info"] = "Invalid Format for FindImage";
                    error(return_error);
                }
            }

            std::vector<unsigned char> img_enc;
            img_enc = img.get_encoded_image(format);

            if (!img_enc.empty()) {

                std::string* img_str = query_res.add_blobs();
                img_str->resize(img_enc.size());
                std::memcpy((void*)img_str->data(),
                            (void*)img_enc.data(),
                            img_enc.size());
            }
            else {
                Json::Value return_error;
                return_error["status"]  = RSCommand::Error;
                return_error["info"] = "Image Data not found";
                error(return_error);
            }
        } catch (VCL::Exception e) {
            print_exception(e);
            Json::Value return_error;
            return_error["status"]  = RSCommand::Error;
            return_error["info"] = "VCL Exception";
            error(return_error);
        }
    }

    if (!flag_empty) {
        findImage.removeMember("entities");
    }

    ret[_cmd_name].swap(findImage);
    return ret;
}
