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
#include "defines.h"

using namespace VDMS;

//========= AddImage definitions =========

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
        else if (type == "flip") {
            img.flip(get_value<int>(op, "code"));
        }
        else if (type == "rotate") {
            img.rotate(get_value<double>(op, "angle"),
                       get_value<bool>(op, "resize"));
        }
        else if (type == "custom") {
            custom_vcl_function(img, ops);
        }
        else {
            throw ExceptionCommand(ImageError, "Operation not defined");
        }
    }
}

VCL::Image::Format ImageCommand::get_requested_format(const Json::Value& cmd)
{
    VCL::Image::Format format;

    std::string requested_format = get_value<std::string>(cmd, "format", "");

    if (requested_format == "png") {
        return VCL::Image::Format::PNG;
    }
    if (requested_format == "jpg") {
        return VCL::Image::Format::JPG;
    }
    if (requested_format == "tdb") {
        return VCL::Image::Format::TDB;
    }
    if (requested_format == "bin") {
        return VCL::Image::Format::BIN;
    }

    return VCL::Image::Format::NONE_IMAGE;
}

//========= AddImage definitions =========

AddImage::AddImage() : ImageCommand("AddImage")
{
    _storage_tdb = VDMSConfig::instance()->get_path_tdb();
    _storage_png = VDMSConfig::instance()->get_path_png();
    _storage_jpg = VDMSConfig::instance()->get_path_jpg();
    _storage_bin = VDMSConfig::instance()->get_path_bin();
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


    std::string format = get_value<std::string>(cmd, "format", "");
    char binary_img_flag = 0;
    if(format == "bin")
    {
        binary_img_flag = 1;
    }
 
    VCL::Image img((void*)blob.data(), blob.size(), binary_img_flag);
    if (cmd.isMember("operations")) {
        enqueue_operations(img, cmd["operations"]);
    }

    std::string img_root = _storage_tdb;
    VCL::Image::Format vcl_format = img.get_image_format();

    if (cmd.isMember("format")) {

        if (format == "png") {
            vcl_format = VCL::Image::Format::PNG;
            img_root = _storage_png;
        }
        else if (format == "tdb") {
            vcl_format = VCL::Image::Format::TDB;
            img_root = _storage_tdb;
        }
        else if (format == "jpg") {
            vcl_format = VCL::Image::Format::JPG;
            img_root = _storage_jpg;
        }
        else if (format == "bin") {
            vcl_format = VCL::Image::Format::BIN;
            img_root = _storage_bin;
        }
        else {
            error["info"] = format + ": format not implemented";
            error["status"] = RSCommand::Error;
            return -1;
        }
    }

    std::string file_name = VCL::create_unique(img_root, format);

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
        add_link(query, cmd["link"], node_ref, VDMS_IM_EDGE_TAG);
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

    // Update Image node
    query.UpdateNode(get_value<int>(cmd, "_ref", -1),
                     VDMS_IM_TAG,
                     cmd["properties"],
                     cmd["remove_props"],
                     cmd["constraints"],
                     get_value<bool>(cmd, "unique", false));

    return 0;
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
    protobufs::queryMessage &query_res,
    const std::string &blob)
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
        return error(return_error);
    }

    Json::Value& findImage = responses[0];

    if (findImage["status"] != 0) {
        findImage["status"]  = RSCommand::Error;
        // Uses PMGD info error.
        return error(findImage);
    }

    Json::Value results = get_value<Json::Value>(cmd, "results");

    bool flag_empty = false;

    // Check if blob (image) must be returned
    if (get_value<bool>(results, "blob", true)) {

        for (auto& ent : findImage["entities"]) {

            assert(ent.isMember(VDMS_IM_PATH_PROP));

            std::string im_path = ent[VDMS_IM_PATH_PROP].asString();
            ent.removeMember(VDMS_IM_PATH_PROP);

            if (ent.getMemberNames().size() == 0) {
                flag_empty = true;
            }

            try {
                VCL::Image img(im_path);

                if (cmd.isMember("operations")) {
                    enqueue_operations(img, cmd["operations"]);
                }

                // We will return the image in the format the user
                // request, or on its format in disk, except for the case
                // of .tdb, where we will encode as png.
                VCL::Image::Format format =
                            img.get_image_format() != VCL::Image::Format::TDB ?
                            img.get_image_format() : VCL::Image::Format::PNG;

                if (cmd.isMember("format")) {
                    format = get_requested_format(cmd);
                    if (format == VCL::Image::Format::NONE_IMAGE ||
                        format == VCL::Image::Format::TDB) {
                        Json::Value return_error;
                        return_error["status"]  = RSCommand::Error;
                        return_error["info"] = "Invalid Requested Format for FindImage";
                        return error(return_error);
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
                    return error(return_error);
                }
            } catch (VCL::Exception e) {
                print_exception(e);
                Json::Value return_error;
                return_error["status"]  = RSCommand::Error;
                return_error["info"] = "VCL Exception";
                return error(return_error);
            }
        }
    }

    if (flag_empty) {
        findImage.removeMember("entities");
    }

    ret[_cmd_name].swap(findImage);
    return ret;
}
