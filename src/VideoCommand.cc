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
#include <fstream>

#include "VideoCommand.h"
#include "VDMSConfig.h"
#include "defines.h"

using namespace VDMS;

VideoCommand::VideoCommand(const std::string &cmd_name):
    RSCommand(cmd_name)
{
}

void VideoCommand::enqueue_operations(VCL::Video& video, const Json::Value& ops)
{
    // Correct operation type and parameters are guaranteed at this point
    for (auto& op : ops) {
        const std::string& type = get_value<std::string>(op, "type");
         std::string unit ;
        if (type == "threshold") {
            video.threshold(get_value<int>(op, "value"));

        }
        else if (type == "interval") {

            video.interval(
                VCL::Video::FRAMES,
                get_value<int>(op, "start"),
                get_value<int>(op, "stop"),
                get_value<int>(op, "step"));

        }
        else if (type == "resize") {
             video.resize(get_value<int>(op, "height"),
                          get_value<int>(op, "width") );

        }
        else if (type == "crop") {
            video.crop(VCL::Rectangle (
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

VCL::Video::Codec VideoCommand::string_to_codec(const std::string& codec)
{
    if (codec == "h263") {
        return VCL::Video::Codec::H263;
    }
    else if (codec == "xvid") {
        return VCL::Video::Codec::XVID;
    }
    else if (codec == "h264") {
        return VCL::Video::Codec::H264;
    }

    return VCL::Video::Codec::NOCODEC;
}

//========= AddVideo definitions =========

AddVideo::AddVideo() : VideoCommand("AddVideo")
{
    _storage_video = VDMSConfig::instance()->get_path_videos();
}

int AddVideo::construct_protobuf(
    PMGDQuery& query,
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

    // The container and codec are checked by the schema.
    // We default to mp4 and h264, if not specified
    const std::string& container =
                            get_value<std::string>(cmd, "container", "mp4");
    const std::string& file_name =
                            VCL::create_unique(_storage_video, container);

    // Modifiyng the existing properties that the user gives
    // is a good option to make the AddNode more simple.
    // This is not ideal since we are manupulating with user's
    // input, but for now it is an acceptable solution.
    Json::Value props = get_value<Json::Value>(cmd, "properties");
    props[VDMS_VID_PATH_PROP] = file_name;

    // Add Video node
    query.AddNode(node_ref, VDMS_VID_TAG, props, Json::Value());

    const std::string& codec = get_value<std::string>(cmd, "codec", "h264");
    VCL::Video::Codec vcl_codec = string_to_codec(codec);

    video.store(file_name, vcl_codec);

    // In case we need to cleanup the query
    error["video_added"] = file_name;

    if (cmd.isMember("link")) {
        add_link(query, cmd["link"], node_ref, VDMS_VID_EDGE);
    }

    return 0;
}

//========= UpdateImage definitions =========

UpdateVideo::UpdateVideo() : VideoCommand("UpdateVideo")
{
}

int UpdateVideo::construct_protobuf(
    PMGDQuery& query,
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
    query.UpdateNode(node_ref, VDMS_VID_TAG, props,
                        remove_props,
                        constraints,
                        get_value<bool>(cmd, "unique", false));

    return 0;
}

Json::Value UpdateVideo::construct_responses(
    Json::Value& responses,
    const Json::Value& json,
    protobufs::queryMessage &query_res,
    const std::string &blob)
{
    assert(responses.size() == 1);

    Json::Value ret;

    // TODO In order to support "codec" or "operations", we could
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

    // Unless otherwhise specified, we return the blob.
    if (get_value<bool>(results, "blob", true)){
        results["list"].append(VDMS_VID_PATH_PROP);
    }

    query.QueryNode(
            get_value<int>(cmd, "_ref", -1),
            VDMS_VID_TAG,
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

        if(!ent.isMember(VDMS_VID_PATH_PROP)){
            continue;
        }

        std::string video_path = ent[VDMS_VID_PATH_PROP].asString();
        ent.removeMember(VDMS_VID_PATH_PROP);

        if (ent.getMemberNames().size() > 0) {
            flag_empty = false;
        }
        try {
            if (!cmd.isMember("operations") &&
                !cmd.isMember("container")  &&
                !cmd.isMember("codec"))
            {
                // Return video as is.
                std::ifstream ifile(video_path, std::ifstream::in);
                ifile.seekg(0, std::ios::end);
                size_t encoded_size = (long)ifile.tellg();
                ifile.seekg(0, std::ios::beg);

                std::string* video_str = query_res.add_blobs();
                video_str->resize(encoded_size);
                ifile.read((char*)(video_str->data()), encoded_size);
                ifile.close();
            }
            else {

                VCL::Video video(video_path);

                if (cmd.isMember("operations")) {
                    enqueue_operations(video, cmd["operations"]);
                }

                const std::string& container =
                            get_value<std::string>(cmd, "container", "mp4");
                const std::string& file_name =
                            VCL::create_unique("/tmp/", container);
                const std::string& codec =
                            get_value<std::string>(cmd, "codec", "h264");

                VCL::Video::Codec vcl_codec = string_to_codec(codec);
                video.store(file_name, vcl_codec); // to /tmp/ for encoding.

                auto video_enc = video.get_encoded();
                int size = video_enc.size();

                if (size > 0) {

                    std::string* video_str = query_res.add_blobs();
                    video_str->resize(size);
                    std::memcpy((void*)video_str->data(),
                                (void*)video_enc.data(),
                                size);
                }
                else {
                    Json::Value return_error;
                    return_error["status"]  = RSCommand::Error;
                    return_error["info"] = "Video Data not found";
                    error(return_error);
                }
            }
        } catch (VCL::Exception e) {
            print_exception(e);
            Json::Value return_error;
            return_error["status"]  = RSCommand::Error;
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
