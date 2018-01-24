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
#include "AthenaConfig.h"

using namespace athena;

#define ATHENA_IM_TAG           "AT:IMAGE"
#define ATHENA_IM_PATH_PROP     "imgPath"
#define ATHENA_IM_EDGE          "AT:IMG_LINK"

//========= AddImage definitions =========

ImageCommand::ImageCommand(const std::string &cmd_name):
    RSCommand(cmd_name)
{
}

void ImageCommand::run_operations(VCL::Image& vclimg, const Json::Value& ops)
{
    // Correct operation type and parameters are guaranteed at this point
    for (auto& op : ops) {
        std::string type = get_value<std::string>(op, "type");
        if (type == "threshold") {
            vclimg.threshold(get_value<int>(op, "value"));
        }
        else if (type == "resize") {
            vclimg.resize(get_value<int>(op, "height"),
                          get_value<int>(op, "width") );
        }
        else if (type == "crop") {
            vclimg.crop(VCL::Rectangle (
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

AddImage::AddImage() : ImageCommand("AddImage")
{
    _storage_tdb = AthenaConfig::instance()
                ->get_string_value("tiledb_database", DEFAULT_TDB_PATH);
    _storage_png = AthenaConfig::instance()
                ->get_string_value("png_database", DEFAULT_PNG_PATH);
    _storage_jpg = AthenaConfig::instance()
                ->get_string_value("jpg_database", DEFAULT_JPG_PATH);
}

int AddImage::construct_protobuf(PMGDQuery& query,
    const Json::Value& jsoncmd,
    const std::string& blob,
    int grp_id,
    Json::Value& error)
{
    const Json::Value &cmd = jsoncmd[_cmd_name];

    int node_ref = get_value<int>(cmd, "_ref", ATOMIC_ID.fetch_add(1));

    VCL::Image vclimg((void*)blob.data(), blob.size());

    if (cmd.isMember("operations")) {
        run_operations(vclimg, cmd["operations"]);
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

    std::string file_name = vclimg.create_unique(img_root, vcl_format);

    Json::Value props =
            get_value<Json::Value>(cmd, "properties");
    props[ATHENA_IM_PATH_PROP] = file_name;

    // Add Image node
    query.AddNode(node_ref, ATHENA_IM_TAG, props, Json::Value());

    vclimg.store(file_name, vcl_format);

    if (cmd.isMember("link")) {
        const Json::Value& link = cmd["link"];

        if (link.isMember("ref")) {

            int src, dst;
            if (link.isMember("direction") && link["direction"] == "in") {
                src = get_value<int>(link,"ref");
                dst = node_ref;
            }
            else {
                dst = get_value<int>(link,"ref");
                src = node_ref;
            }

            query.AddEdge(-1, src, dst,
                get_value<std::string>(link, "class", ATHENA_IM_EDGE),
                get_value<Json::Value>(link, "properties")
                );
        }
    }

    if (cmd.isMember("collections")) {
        Json::Value collections = cmd["collections"];

        for (auto col : collections) {

            int col_ref = ATOMIC_ID.fetch_add(1);

            std::string col_tag = ATHENA_COL_TAG;

            Json::Value props_col;
            props_col[ATHENA_COL_NAME_PROP] = col.asString();

            Json::Value constraints;
            Json::Value arr;
            arr.append("==");
            arr.append(col.asString());
            constraints[ATHENA_COL_NAME_PROP] = arr;

            bool unique = true;

            // Conditional adding node
            query.AddNode(col_ref, col_tag, props_col, constraints, unique);

            // Add edge between collection and image
            query.AddEdge(-1, col_ref, node_ref, ATHENA_COL_EDGE_TAG, props_col);
        }
    }

    return 0;
}

Json::Value AddImage::construct_responses(
    Json::Value& response,
    const Json::Value &json,
    protobufs::queryMessage &query_res)
{
    Json::Value resp = check_responses(response);

    Json::Value ret;
    ret[_cmd_name] = resp;
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
    const Json::Value &cmd = jsoncmd[_cmd_name];

    Json::Value results =
                    get_value<Json::Value>(cmd, "results");
    results["list"].append(ATHENA_IM_PATH_PROP);

    // if (find_img.isMember("collections")) {
    //     Json::Value collections = find_img["collections"];
    //     for (auto col : collections) {
    //         // Do stuff with the collections
    //         // Here we will need and/or etc.
    //         // Need PMGD support for this.
    //     }
    // }

    query.QueryNode(
            get_value<int>(cmd, "_ref", -1),
            ATHENA_IM_TAG,
            get_value<Json::Value>(cmd, "link"),
            get_value<Json::Value>(cmd, "constraints"),
            results,
            get_value<bool>(cmd, "unique", false)
            );

    return 0;
}

Json::Value FindImage::construct_responses(
    Json::Value &responses,
    const Json::Value &json,
    protobufs::queryMessage &query_res)
{
    Json::Value findImage;
    const Json::Value &cmd = json[_cmd_name];

    Json::Value ret;

    bool flag_error = false;

    if (responses.size() == 0) {
        findImage["status"]  = RSCommand::Error;
        findImage["info"] = "Not Found!";
        flag_error = true;
        ret[_cmd_name] = findImage;
        return ret;
    }

    for (auto res : responses) {

        if (res["status"] != 0) {
            flag_error = true;
            break;
        }

        if (!res.isMember("entities"))
            continue;

        findImage = res;

        for (auto& ent : findImage["entities"]) {

            assert(ent.isMember(ATHENA_IM_PATH_PROP));
            std::string im_path = ent[ATHENA_IM_PATH_PROP].asString();
            try {
                VCL::Image vclimg(im_path);

                if (cmd.isMember("operations")) {
                    run_operations(vclimg, cmd["operations"]);
                }

                std::vector<unsigned char> img_enc;
                img_enc = vclimg.get_encoded_image(VCL::PNG);
                if (!img_enc.empty()) {
                    std::string img_str((const char*)
                                        img_enc.data(),
                                        img_enc.size());

                    query_res.add_blobs(img_str);
                }
            } catch (VCL::Exception e) {
                print_exception(e);
                findImage["status"] = RSCommand::Error;
                findImage["info"]   = "VCL Exception";
                flag_error = true;
                break;
            }
        }
    }

    if (!flag_error) {
        findImage["status"] = RSCommand::Success;
    }

    // In case no properties asked by the user
    // TODO: This is more like a hack. I don;t like it
    bool empty_flag = false;

    for (auto& ent : findImage["entities"]) {
        ent.removeMember(ATHENA_IM_PATH_PROP);
        if (ent.getMemberNames().size() == 0) {
            empty_flag = true;
            break;
        }
    }

    if (empty_flag) {
        findImage.removeMember("entities");
    }

    ret[_cmd_name] = findImage;

    return ret;
}
