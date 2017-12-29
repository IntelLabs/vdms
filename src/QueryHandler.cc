#include <string>
#include <fstream>
#include "QueryHandler.h"
#include "chrono/Chrono.h"
#include "ExceptionsCommand.h"

#include "jarvis.h"
#include "util.h"

#include "AthenaConfig.h"

#include <jsoncpp/json/json.h>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/writer.h>

#define ATHENA_IM_TAG           "AT:IMAGE"
#define ATHENA_IM_PATH_PROP     "imgPath"
#define ATHENA_IM_EDGE          "AT:IMG_LINK"

#define ATHENA_COL_TAG          "AT:COLLECTION"
#define ATHENA_COL_NAME_PROP    "name"
#define ATHENA_COL_EDGE_TAG     "collection_tag"

#define PARAM_MANDATORY 2
#define PARAM_OPTIONAL  1

using namespace athena;

static uint32_t STATIC_IDENTIFIER = 0;

// TODO This will be later replaced by a real logger
std::ofstream GENERIC_LOGGER("log.log", std::fstream::app);
// #define GENERIC_LOGGER std::cout

QueryHandler::QueryHandler(Jarvis::Graph *db, std::mutex *mtx)
    : _pmgd_qh(db, mtx)
{
    _rs_cmds["AddEntity"]  = new AddEntity();
    _rs_cmds["Connect"]    = new Connect();
    _rs_cmds["FindEntity"] = new FindEntity();
    _rs_cmds["AddImage"]   = new AddImage();
    _rs_cmds["FindImage"]  = new FindImage();
}

QueryHandler::~QueryHandler()
{
    for (auto cmd: _rs_cmds) {
        delete cmd.second;
    }
}

void QueryHandler::process_connection(comm::Connection *c)
{
    CommandHandler handler(c);

    try {
        while (true) {
            protobufs::queryMessage response;
            protobufs::queryMessage query = handler.get_command();
            process_query(query, response);
            handler.send_response(response);
        }
    } catch (comm::ExceptionComm e) {
        print_exception(e);
    }
    delete c;
}

bool QueryHandler::syntax_checker(const Json::Value &root, Json::Value& error)
{
    for (int j = 0; j < root.size(); j++) {
        const Json::Value& query = root[j];
        if (query.getMemberNames().size() != 1) {
            error["info"] = "Error: Only one command per element allowed";
            return false;
        }

        const std::string cmd_str = query.getMemberNames()[0];
        auto it = _rs_cmds.find(cmd_str);
        if (it == _rs_cmds.end()) {
            error["info"] = cmd_str + ": Command not found!";
            return false;
        }

        bool flag_error = _rs_cmds[cmd_str]->check_params(query[cmd_str],
                                                          error);

        if (!flag_error) {
            return false;
        }
    }

    return true;
}

int QueryHandler::parse_commands(const std::string& commands,
                                 Json::Value& root)
{
    Json::Reader reader;

    try {
        bool parseSuccess = reader.parse(commands.c_str(), root);

        if (!parseSuccess) {
            root["info"] = "Error parsing the query, ill formed JSON";
            root["status"] = RSCommand::Error;
            return -1;
        }

        Json::Value error;

        if (!syntax_checker(root, error)) {
            root = error;
            root["status"] = RSCommand::Error;
            return -1;
        }

        // Json::StyledWriter swriter;
        // GENERIC_LOGGER << swriter.write(root) << std::endl;
    } catch (Json::Exception const&) {
        root["info"] = "Json Exception at Parsing";
        root["status"] = RSCommand::Error;
        return -1;
    }

    return 0;
}

void QueryHandler::process_query(protobufs::queryMessage& proto_query,
                                 protobufs::queryMessage& proto_res)
{
    Json::FastWriter fastWriter;

    try {
        Json::Value json_responses;
        Json::Value root;

        Json::Value cmd_result;
        Json::Value cmd_current;
        bool flag_error = false;

        if (parse_commands(proto_query.json(), root) != 0) {
            cmd_result = root;
            flag_error = true;
            cmd_current = "Transaction";
        }

        PMGDTransaction tx(_pmgd_qh);
        unsigned blob_count = 0;

        //iterate over the list of the queries
        for (int j = 0; j < root.size() && !flag_error; j++) {
            const Json::Value& query = root[j];
            assert (query.getMemberNames().size() == 1);
            std::string cmd = query.getMemberNames()[0];

            int group_count = tx.add_group();
            int ret_code;

            if (_rs_cmds[cmd]->need_blob()) {
                assert (proto_query.blobs().size() >= blob_count);
                std::string blob = proto_query.blobs(blob_count);

                ret_code = _rs_cmds[cmd]->construct_protobuf(tx, query, blob,
                        group_count, cmd_result);
                blob_count++;
            }
            else {
                ret_code = _rs_cmds[cmd]->construct_protobuf(tx, query, "",
                        group_count, cmd_result);
            }

            if (ret_code != 0) {
                flag_error = true;
                cmd_current = root[j];
                break;
            }
        }

        if (!flag_error) {
            Json::Value& tx_responses = tx.run();

            if (tx_responses.size() != root.size()) { // error
                flag_error = true;
                cmd_current = "Transaction";
                cmd_result = tx_responses;
                cmd_result["info"] = "Failed PMGDTransaction";
                cmd_result["status"] = RSCommand::Error;
            } else {

                for (int j = 0; j < root.size(); j++) {
                    std::string cmd = root[j].getMemberNames()[0];

                    cmd_result = _rs_cmds[cmd]->construct_responses(
                                                tx_responses[j],
                                                root[j], proto_res);

                    // This is for error handling
                    if (cmd_result.isMember("status")) {
                        int status = cmd_result["status"].asInt();
                        if (status != RSCommand::Success ||
                            status != RSCommand::Empty   ||
                            status != RSCommand::Exists)
                        {
                            flag_error = true;
                            cmd_current = root[j];
                            break;
                        }
                    }
                    json_responses.append(cmd_result);
                }
            }
        }

        if (flag_error) {
            cmd_result["FailedCommand"] = cmd_current;
            json_responses.clear();
            json_responses.append(cmd_result);
            proto_res.clear_blobs();
            Json::StyledWriter w;
            GENERIC_LOGGER << w.write(json_responses);
        }

        proto_res.set_json(fastWriter.write(json_responses));

    } catch (VCL::Exception e) {
        print_exception(e);
        GENERIC_LOGGER << "FATAL ERROR: VCL Exception at QH" << std::endl;
        exit(0);
    } catch (Jarvis::Exception e) {
        print_exception(e);
        GENERIC_LOGGER << "FATAL ERROR: PMGD Exception at QH" << std::endl;
        exit(0);
    } catch (ExceptionCommand e) {
        print_exception(e);
        GENERIC_LOGGER << "FATAL ERROR: Command Exception at QH" << std::endl;
        exit(0);
    } catch (Json::Exception const&) {
        // Should not happen
        // In case of error on the last fastWriter
        GENERIC_LOGGER << "FATAL: Json Exception!" << std::endl;
        Json::Value error;
        error["info"] = "Internal Server Error: Json Exception";
        error["status"] = RSCommand::Error;
        proto_res.set_json(fastWriter.write(error));
    } catch (const std::invalid_argument& ex) {
        GENERIC_LOGGER << "Invalid argument: " << ex.what() << '\n';
        exit(0);
    }
}

//========= AddEntity definitions =========

AddEntity::AddEntity() : RSCommand("AddEntity")
{
    _valid_params_map["class"]       = PARAM_MANDATORY;
    _valid_params_map["_ref"]        = PARAM_OPTIONAL;
    _valid_params_map["properties"]  = PARAM_OPTIONAL;
    _valid_params_map["constraints"] = PARAM_OPTIONAL;
}

int AddEntity::construct_protobuf(PMGDTransaction& tx,
    const Json::Value& jsoncmd,
    const std::string& blob,
    int grp_id,
    Json::Value& error)
{
    const Json::Value &cmd = jsoncmd[_cmd_name];

    std::string tag = cmd["class"].asString();

    int node_ref = -1;
    if (cmd.isMember("_ref")) {
        node_ref = cmd["_ref"].asInt();
    }

    Json::Value props;
    if (cmd.isMember("properties")) {
        props = cmd["properties"];
    }

    // Check if conditional add
    bool unique = false;
    if (cmd.isMember("unique")) {
        unique = cmd["unique"].asBool();
    }

    Json::Value constraints;
    if (cmd.isMember("constraints")) {
        constraints = cmd["constraints"];
    }

    tx.AddNode(node_ref, tag, props, constraints, unique);

    return 0;
}

Json::Value AddEntity::construct_responses(
    Json::Value& response,
    const Json::Value &json,
    protobufs::queryMessage &query_res)
{
    assert(response.size() == 1);

    Json::Value addEntity;
    addEntity[_cmd_name] = response[0];
    return addEntity;
}

//========= Connect definitions =========

Connect::Connect() : RSCommand("Connect")
{
    _valid_params_map["ref1"]       = PARAM_MANDATORY;
    _valid_params_map["ref2"]       = PARAM_MANDATORY;
    _valid_params_map["class"]      = PARAM_OPTIONAL;
    _valid_params_map["properties"] = PARAM_OPTIONAL;
}

int Connect::construct_protobuf(
        PMGDTransaction& tx,
        const Json::Value& jsoncmd,
        const std::string& blob,
        int grp_id,
        Json::Value& error)
{
    const Json::Value &cmd = jsoncmd[_cmd_name];

    int edge_ref = -1;
    if (cmd.isMember("_ref")) {
        edge_ref = cmd["_ref"].asInt();
    }

    Json::Value props;
    if (cmd.isMember("properties")) {
        props = cmd["properties"];
    }

    int src = cmd["ref1"].asInt();
    int dst = cmd["ref2"].asInt();
    const std::string &tag = cmd["class"].asString();

    tx.AddEdge(edge_ref, src, dst, tag, props);
    return 0;
}

Json::Value Connect::construct_responses(
    Json::Value& response,
    const Json::Value &json,
    protobufs::queryMessage &query_res)
{
    assert(response.size() == 1);

    Json::Value ret;
    ret[_cmd_name] = response[0];
    return ret;
}

//========= FindEntity definitions =========

FindEntity::FindEntity() : RSCommand("FindEntity")
{
    _valid_params_map["class"]       = PARAM_OPTIONAL;
    _valid_params_map["_ref"]        = PARAM_OPTIONAL;
    _valid_params_map["constraints"] = PARAM_OPTIONAL;
    _valid_params_map["results"]     = PARAM_OPTIONAL;
    _valid_params_map["unique"]      = PARAM_OPTIONAL;
    _valid_params_map["link"]        = PARAM_OPTIONAL;
}

int FindEntity::construct_protobuf(
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

    const std::string& tag = cmd["class"].asString();

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

    tx.QueryNode(node_ref, tag, link, constraints, results, unique);

    return 0;
}

Json::Value FindEntity::construct_responses(
    Json::Value& response,
    const Json::Value &json,
    protobufs::queryMessage &query_res)
{
    assert(response.size() == 1);

    Json::Value ret;
    ret[_cmd_name] = response[0];
    return ret;
}

//========= AddImage definitions =========

AddImage::AddImage() : RSCommand("AddImage")
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

FindImage::FindImage() : RSCommand("FindImage")
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
