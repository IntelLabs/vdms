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

#include <iostream>

#include "VideoCommand.h"
#include "VDMSConfig.h"

using namespace VDMS;

#define VDMS_VIDEO_TAG           "VDMS:Video"
#define VDMS_VIDEO_EDGE          "VDMS:VideoLINK"
#define VDMS_VIDEO_PATH_PROP     "VDMS:VideoPath"

VideoCommand::VideoCommand(const std::string &cmd_name):
    RSCommand(cmd_name)
{
}

void VideoCommand::enqueue_operations(VCL::Video& video, const Json::Value& ops)
{
    // Correct operation type and parameters are guaranteed at this point
    for (auto& op : ops) {
        const std::string& type = get_value<std::string>(op, "type");
        if (type == "threshold") {
            video.threshold(get_value<int>(op, "value"),
                            get_value<int>(op, "start"),
                            get_value<int>(op, "stop"),
                            get_value<int>(op, "step") );
        }
        else if (type == "interval") {
        //   video.interval(get_value<int>(op, "from"),
         //                  get_value<int>(op, "to"));
        }
        else if (type == "resize") {
             video.resize(get_value<int>(op, "height"),
                           get_value<int>(op, "width") ,
                           get_value<int>(op, "start"),
                          get_value<int>(op, "stop"),
                          get_value<int>(op, "step"));
        }
        else if (type == "crop") {
            video.crop(VCL::Rectangle (
                        get_value<int>(op, "x"),
                        get_value<int>(op, "y"),
                        get_value<int>(op, "width"),
                        get_value<int>(op, "height") ),
                        get_value<int>(op, "start"),
                        get_value<int>(op, "stop"),
                        get_value<int>(op, "step")

            );
        }
        else {
            throw ExceptionCommand(ImageError, "Operation not defined");
        }
    }
}

//========= AddVideo definitions =========

AddVideo::AddVideo() : VideoCommand("AddVideo")
{

    _storage_video = VDMSConfig::instance()
                  ->get_string_value("video_path", DEFAULT_VIDEO_PATH);
    // _storage_png = VDMSConfig::instance()
    //             ->get_string_value("png_path", DEFAULT_PNG_PATH);
    // _storage_jpg = VDMSConfig::instance()
    //             ->get_string_value("jpg_path", DEFAULT_JPG_PATH);
}

int AddVideo::construct_protobuf(PMGDQuery& query,
    const Json::Value& jsoncmd,
    const std::string& blob,
    int grp_id,
    Json::Value& error)
{
    const Json::Value& cmd = jsoncmd[_cmd_name];

    int node_ref = get_value<int>(cmd, "_ref",
                                  query.get_available_reference());

    VCL::Video video((void*)blob.data(), blob.size());

    if (cmd.isMember("operations")) {
        enqueue_operations(video, cmd["operations"]);
    }

    std::string video_root = _storage_video;
    VCL::Format vcl_format  = VCL::Format::AVI;

    if (cmd.isMember("format")) {
        std::string format = get_value<std::string>(cmd, "format");

        if (format == "mp4") {
            vcl_format = VCL::Format::MP4;
            video_root = _storage_video;
        }
        else if (format == "avi") {
            vcl_format = VCL::Format::AVI;
            video_root = _storage_video;
        }
        else if (format == "mpeg") {
            vcl_format = VCL::Format::MPEG;
            video_root = _storage_video;
        }

        else {
            error["info"] = format + ": format not implemented";
            error["status"] = RSCommand::Error;
            return -1;
        }
    }

    std::string file_name  = video.create_unique(video_root, vcl_format);

    // Modifiyng the existing properties that the user gives
    // is a good option to make the AddNode more simple.
    // This is not ideal since we are manupulating with user's
    // input, but for now it is an acceptable solution.
    Json::Value props = get_value<Json::Value>(cmd, "properties");
    props[VDMS_VIDEO_PATH_PROP] = file_name;

    // Add Video node
    query.AddNode(node_ref, VDMS_VIDEO_TAG, props, Json::Value());

    video.store(file_name, vcl_format, 10, 100, 1);

    // In case we need to cleanup the query
    error["video_added"] = file_name;

    if (cmd.isMember("link")) {
        add_link(query, cmd["link"], node_ref, VDMS_VIDEO_EDGE);
    }

    return 0;
}
//=========AddFrame definitions ===========
AddFrame::AddFrame() : VideoCommand("AddFrame")
{

}

int AddFrame::construct_protobuf(PMGDQuery& query,
    const Json::Value& jsoncmd,
    const std::string& blob,
    int grp_id,
    Json::Value& error)
{
    const Json::Value& cmd = jsoncmd[_cmd_name];

    int node_ref = get_value<int>(cmd, "_ref",
                                  query.get_available_reference());



    if (cmd.isMember("operations")) {
       // enqueue_operations(video, cmd["operations"]);
    }

    std::string video_root; // = _storage_tdb;
    VCL::Format vcl_format ;// = VCL::TDB;

    if (cmd.isMember("format")) {
        std::string format = get_value<std::string>(cmd, "format");

        if (format == "mp4") {
            vcl_format = VCL::Format::MP4;
            video_root = _storage_video;
        }
        else if (format == "avi") {
            vcl_format = VCL::Format::AVI;
            video_root = _storage_video;
        }
        else if (format == "mpeg") {
            vcl_format = VCL::Format::MPEG;
            video_root = _storage_video;
        }

        else {
            error["info"] = format + ": format not implemented";
            error["status"] = RSCommand::Error;
            return -1;
        }

    }
    VCL::Video video((void*)blob.data(), blob.size());
    std::string file_name = video.create_unique(video_root, vcl_format);

    // Modifiyng the existing properties that the user gives
    // is a good option to make the AddNode more simple.
    // This is not ideal since we are manupulating with user's
    // input, but for now it is an acceptable solution.
    Json::Value props = get_value<Json::Value>(cmd, "properties");
    props[VDMS_VIDEO_PATH_PROP] = file_name;

    // Add Image node
    query.AddNode(node_ref, VDMS_VIDEO_TAG, props, Json::Value());

    video.store(file_name, vcl_format);

    // In case we need to cleanup the query
    error["frame_added"] = file_name;

    if (cmd.isMember("link")) {
        add_link(query, cmd["link"], node_ref, VDMS_VIDEO_EDGE);
    }

    return 0;
}

//========= UpdateImage definitions =========

UpdateVideo::UpdateVideo() : VideoCommand("UpdateVideo")
{

}

int UpdateVideo::construct_protobuf(PMGDQuery& query,
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
    query.UpdateNode(node_ref, VDMS_VIDEO_TAG, props,
                        remove_props,
                        constraints,
                        get_value<bool>(cmd, "unique", false));

    return 0;
}

Json::Value UpdateVideo::construct_responses(
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

//========= FindVideo definitions =========

FindVideo::FindVideo() : VideoCommand("FindVideo")
{

}

int FindVideo::construct_protobuf(
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
        results["list"].append(VDMS_VIDEO_PATH_PROP);
    }

    query.QueryNode(
            get_value<int>(cmd, "_ref", -1),
            VDMS_VIDEO_TAG,
            cmd["link"],
            cmd["constraints"],
            results,
            get_value<bool>(cmd, "unique", false)
            );

    return 0;
}

Json::Value FindVideo::construct_responses(
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

    Json::Value& FindVideo = responses[0];

    assert(FindVideo.isMember("entities"));

    if (FindVideo["status"] != 0) {
        FindVideo["status"]  = RSCommand::Error;
        // Uses PMGD info error.
        error(FindVideo);
    }

    bool flag_empty = true;

    for (auto& ent : FindVideo["entities"]) {

        if(!ent.isMember(VDMS_VIDEO_PATH_PROP)){
            continue;
        }
        std::string im_path = ent[VDMS_VIDEO_PATH_PROP].asString();
        ent.removeMember(VDMS_VIDEO_PATH_PROP);

        if (ent.getMemberNames().size() > 0) {
            flag_empty = false;
        }

        try {
            VCL::Video video(im_path);

            if (cmd.isMember("operations")) {
                enqueue_operations(video, cmd["operations"]);
            }


            VCL::Format format;
            if (cmd.isMember("format")) {
                std::string requested_format =
                            get_value<std::string>(cmd, "format");

                if (requested_format == "mp4") {
                    format = VCL::Format::MP4;
                }
                else if (requested_format == "avi") {
                    format = VCL::Format::AVI;
                }
                else if (requested_format == "mpeg") {
                    format = VCL::Format::MPEG;
                }
                else {
                    Json::Value return_error;
                    return_error["status"]  = RSCommand::Error;
                    return_error["info"] = "Invalid Format for FindVideo";
                    error(return_error);
                }
            }

            std::vector<unsigned char> video_enc;
           // video_enc = video.get_encoded_image(format);

            if (!video_enc.empty()) {

                std::string* video_str = query_res.add_blobs();
                video_str->resize(video_enc.size());
                std::memcpy((void*)video_str->data(),
                            (void*)video_enc.data(),
                            video_enc.size());
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
        FindVideo.removeMember("entities");
    }

    ret[_cmd_name].swap(FindVideo);
    return ret;
}
//========= FindFrame definitions =========
FindFrame::FindFrame() : VideoCommand("FindVideo")
{

}

int FindFrame::construct_protobuf(
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
        results["list"].append(VDMS_VIDEO_PATH_PROP);
    }

    query.QueryNode(
            get_value<int>(cmd, "_ref", -1),
            VDMS_VIDEO_TAG,
            cmd["link"],
            cmd["constraints"],
            results,
            get_value<bool>(cmd, "unique", false)
            );

    return 0;
}

Json::Value FindFrame::construct_responses(
    Json::Value& responses,
    const Json::Value& json,
    protobufs::queryMessage &query_res)
{

}