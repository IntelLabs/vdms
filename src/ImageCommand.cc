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

void ImageCommand::run_operations(VCL::Image& vclimg, const Json::Value& op)
{
    for (auto& operation : op) {
        std::string type = operation["type"].asString();
        if (type == "threshold") {
            vclimg.threshold(operation["value"].asInt());
        }
        else if (type == "resize") {
            vclimg.resize(operation["height"].asInt(),
                    operation["width" ].asInt());
        }
        else if (type == "crop") {
            vclimg.crop(VCL::Rectangle (
                        operation["x"].asInt(),
                        operation["y"].asInt(),
                        operation["height"].asInt(),
                        operation["width" ].asInt()));
        }
        else {
            throw ExceptionCommand(ImageError, "Operation not defined");
        }
    }
}

AddImage::AddImage() : ImageCommand("AddImage")
{
    _valid_params_map["collections"] = PARAM_OPTIONAL;
    _valid_params_map["properties"]  = PARAM_OPTIONAL;
    _valid_params_map["operations"]  = PARAM_OPTIONAL;
    _valid_params_map["link"]        = PARAM_OPTIONAL;
    _valid_params_map["format"]      = PARAM_OPTIONAL;

    _storage_tdb = AthenaConfig::instance()
                ->get_string_value("tiledb_database", DEFAULT_TDB_PATH);
    _storage_png = AthenaConfig::instance()
                ->get_string_value("png_database", DEFAULT_PNG_PATH);
    _storage_jpg = AthenaConfig::instance()
                ->get_string_value("jpg_database", DEFAULT_JPG_PATH);
}

int AddImage::construct_protobuf(PMGDTransaction& tx,
    const Json::Value& jsoncmd,
    const std::string& blob,
    int grp_id,
    Json::Value& error)
{
    const Json::Value &cmd = jsoncmd[_cmd_name];

    int node_ref;
    if (cmd.isMember("_ref")) {
        node_ref = cmd["_ref"].asInt();
    }
    else {
        node_ref = STATIC_IDENTIFIER++;
    }

    VCL::Image vclimg((void*)blob.data(), blob.size());

    if (cmd.isMember("operations")) {
        run_operations(vclimg, cmd["operations"]);
    }

    std::string img_root = _storage_tdb;
    VCL::ImageFormat vcl_format = VCL::TDB;

    if (cmd.isMember("format")) {
        std::string format = cmd["format"].asString();

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

    Json::Value props;
    if (cmd.isMember("properties")) {
        props = cmd["properties"];
    }
    props[ATHENA_IM_PATH_PROP] = file_name;

    // Add Image node
    tx.AddNode(node_ref, ATHENA_IM_TAG, props);

    vclimg.store(file_name, vcl_format);

    if (cmd.isMember("link")) {
        const Json::Value& link = cmd["link"];

        if (link.isMember("ref")) {

            int src, dst;
            if (link.isMember("direction") && link["direction"] == "in") {
                src = link["ref"].asUInt();
                dst = node_ref;
            }
            else {
                dst = link["ref"].asUInt();
                src = node_ref;
            }

            std::string e_tag = link.isMember("class")?
                                    link["class"].asString() : ATHENA_IM_EDGE;

            Json::Value props_link;
            if (link.isMember("properties")) {
                props_link = link["properties"];
            }

            tx.AddEdge(-1, src, dst, e_tag, props_link);
        }
    }

    if (cmd.isMember("collections")) {
        Json::Value collections = cmd["collections"];

        for (auto col : collections) {

            int col_ref = STATIC_IDENTIFIER++;

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
            tx.AddNode(col_ref, col_tag, props_col, constraints, unique);

            // Add edge between collection and image
            tx.AddEdge(-1, col_ref, node_ref, ATHENA_COL_EDGE_TAG, props_col);
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
    _valid_params_map["_ref"]        = PARAM_OPTIONAL;
    _valid_params_map["constraints"] = PARAM_OPTIONAL;
    _valid_params_map["collections"] = PARAM_OPTIONAL;
    _valid_params_map["operations"]  = PARAM_OPTIONAL;
    _valid_params_map["unique"]      = PARAM_OPTIONAL;
    _valid_params_map["link"]        = PARAM_OPTIONAL;
    _valid_params_map["results"]     = PARAM_OPTIONAL;
}

int FindImage::construct_protobuf(
    PMGDTransaction& tx,
    const Json::Value& jsoncmd,
    const std::string& blob,
    int grp_id,
    Json::Value& error)
{
    const Json::Value &cmd = jsoncmd[_cmd_name];

    int node_ref = -1;
    if (cmd.isMember("_ref")) {
        node_ref = cmd["_ref"].asInt();
    }

    const std::string& tag = ATHENA_IM_TAG;

    bool unique = false;
    if (cmd.isMember("unique")) {
        unique = cmd["unique"].asBool();
    }

    Json::Value link;
    if (cmd.isMember("link")) {
        link = cmd["link"];
    }

    Json::Value constraints;
    if (cmd.isMember("constraints")) {
        constraints = cmd["constraints"];
    }

    Json::Value results;
    if (cmd.isMember("results")) {
        results = cmd["results"];
    }

    results["list"].append(ATHENA_IM_PATH_PROP);

    // if (find_img.isMember("collections")) {
    //     Json::Value collections = find_img["collections"];

    //     for (auto col : collections) {
    //         // Do stuff with the collections
    //         // Here we will need and/or etc.
    //     }
    // }

    tx.QueryNode(node_ref, tag, link, constraints, results, unique);

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
