#include <string>
#include <fstream>
#include "QueryHandler.h"
#include "chrono/Chrono.h"

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

void QueryHandler::process_query(protobufs::queryMessage& proto_query,
                                 protobufs::queryMessage& proto_res)
{
    Json::FastWriter fastWriter;

    try {
        Json::Value json_responses;
        Json::Value root;
        Json::Reader reader;

        bool parseSuccess = reader.parse(proto_query.json().c_str(), root);

        if ( !parseSuccess ) {
            GENERIC_LOGGER << "Error parsing: " << std::endl;
            GENERIC_LOGGER << proto_query.json() << std::endl;
            Json::Value error;
            error["return"] = "Server error - parsing";
            json_responses.append(error);
            proto_res.set_json(fastWriter.write(json_responses));
            return;
        }

        // TODO REMOVE THIS:
        Json::StyledWriter swriter;
        GENERIC_LOGGER << swriter.write(root) << std::endl;

        // define a vector of commands
        // TODO WE NEED TO DELETE EACH ELEMENT AFTER DONE WITH THIS VECTOR!!!
        std::vector<pmgd::protobufs::Command *> cmds;
        unsigned group_count = 0;
        //this command to start a new transaction
        pmgd::protobufs::Command cmdtx;
        //this the protobuf of a new TxBegin
        cmdtx.set_cmd_id(pmgd::protobufs::Command::TxBegin);
        cmdtx.set_cmd_grp_id(group_count); //give it an ID
        cmds.push_back(&cmdtx); //push the creating command to the vector

        unsigned blob_count = 0;

        //iterate over the list of the queries
        for (int j = 0; j < root.size(); j++) {
            const Json::Value& query = root[j];
            assert (query.getMemberNames().size() == 1);
            std::string cmd = query.getMemberNames()[0];
            ++group_count;

            if (_rs_cmds[cmd]->need_blob()) {
                assert (proto_query.blobs().size() >= blob_count);
                std::string blob = proto_query.blobs(blob_count);
                _rs_cmds[cmd]->construct_protobuf(cmds, query, blob,
                        group_count);
                blob_count++;
            }
            else {
                _rs_cmds[cmd]->construct_protobuf(cmds, query, "",
                        group_count);
            }
        }
        ++group_count;

        pmgd::protobufs::Command cmdtxend;
        // Commit here doesn't change anything. Just indicates end of TX
        cmdtxend.set_cmd_id(pmgd::protobufs::Command::TxCommit);
        cmdtxend.set_cmd_grp_id(group_count);
        cmds.push_back(&cmdtxend);

        // execute the queries using the PMGDQueryHandler object
        std::vector<std::vector<pmgdCmdResponse *>>
            pmgd_responses = _pmgd_qh.process_queries(cmds, group_count + 1);

        // Make sure there were no errors
        if (pmgd_responses.size() != group_count + 1) {
            // TODO: This is where we will need request server rollback code.
            std::vector<pmgdCmdResponse *>& res = pmgd_responses.at(0);
            json_responses.append(RSCommand::construct_error_response(res[0]));
        }
        else {
            for (int j = 0; j < root.size(); j++) {
                std::string cmd = root[j].getMemberNames()[0];
                std::vector<pmgdCmdResponse *>& res =
                            pmgd_responses.at(j+1);
                json_responses.append( _rs_cmds[cmd]->construct_responses(
                                                        res,
                                                        root[j],
                                                        proto_res) );
            }
        }
        proto_res.set_json(fastWriter.write(json_responses));

        for (unsigned i = 0; i < pmgd_responses.size(); ++i) {
            for (unsigned j = 0; j < pmgd_responses[i].size(); ++j) {
                if (pmgd_responses[i][j] != NULL)
                    delete pmgd_responses[i][j];
            }
            pmgd_responses[i].clear();
        }
        pmgd_responses.clear();

    } catch (VCL::Exception e) {
        print_exception(e);
        GENERIC_LOGGER << "VCL Exception!" << std::endl;
        Json::Value error;
        error["error"] = "VCL Exception!";
        proto_res.set_json(fastWriter.write(error));
    } catch (Jarvis::Exception e) {
        print_exception(e);
        GENERIC_LOGGER << "Jarvis Exception!" << std::endl;
        Json::Value error;
        error["error"] = "Jarvis Exception!";
        proto_res.set_json(fastWriter.write(error));
    } catch (Json::Exception const&) {
        GENERIC_LOGGER << "Json Exception!" << std::endl;
        Json::Value error;
        error["error"] = "Json Exception!";
        proto_res.set_json(fastWriter.write(error));
    } catch (const std::invalid_argument& ex) {
        GENERIC_LOGGER << "Invalid argument: " << ex.what() << '\n';
    }

}

//========= AddEntity definitions =========

int AddEntity::construct_protobuf(std::vector<pmgd::protobufs::Command*> &cmds,
        const Json::Value& jsoncmd,
        const std::string& blob,
        int grp_id)
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

    cmds.push_back(AddNode(grp_id, node_ref, tag, props, constraints, unique));

    return 0;
}

Json::Value AddEntity::construct_responses(
    std::vector<pmgdCmdResponse *>& response,
    const Json::Value &json,
    protobufs::queryMessage &query_res)
{
    assert(response.size() == 1);

    Json::Value addEntity;
    addEntity[_cmd_name] = parse_response(response[0]);
    return addEntity;
}

//========= Connect definitions =========

int Connect::construct_protobuf(
        std::vector<pmgd::protobufs::Command*> &cmds,
        const Json::Value& jsoncmd,
        const std::string& blob,
        int grp_id)
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

    cmds.push_back(AddEdge(grp_id, edge_ref, src, dst, tag, props));
    return 0;
}

Json::Value Connect::construct_responses(
    std::vector<pmgdCmdResponse*>& response,
    const Json::Value &json,
    protobufs::queryMessage &query_res)
{
    assert(response.size() == 1);

    Json::Value ret;
    ret[_cmd_name] = parse_response(response[0]);
    return ret;
}

//========= FindEntity definitions =========

int FindEntity::construct_protobuf(
    std::vector<pmgd::protobufs::Command*> &cmds,
    const Json::Value& jsoncmd,
    const std::string& blob,
    int grp_id)
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

    cmds.push_back(
        QueryNode(grp_id, node_ref, tag, link, constraints, results, unique));

    return 0;
}

Json::Value FindEntity::construct_responses(
    std::vector<pmgdCmdResponse*>& response,
    const Json::Value &json,
    protobufs::queryMessage &query_res)
{
    assert(response.size() == 1);

    Json::Value ret;
    ret[_cmd_name] = parse_response(response[0]);
    return ret;
}

//========= AddImage definitions =========

AddImage::AddImage()
{
    _storage_tdb = AthenaConfig::instance()
                ->get_string_value("tiledb_database", DEFAULT_TDB_PATH);
    _storage_png = AthenaConfig::instance()
                ->get_string_value("png_database", DEFAULT_PNG_PATH);
    _storage_jpg = AthenaConfig::instance()
                ->get_string_value("jpg_database", DEFAULT_JPG_PATH);
}

int AddImage::construct_protobuf(std::vector<pmgd::protobufs::Command*> &cmds,
        const Json::Value& jsoncmd,
        const std::string& blob,
        int grp_id)
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
            GENERIC_LOGGER << "Format Not Implemented" << std::endl;
            Json::Value error;
            error["Format"] = format + " Not implemented";
            // response.append(error);
            // return;
        }
    }

    std::string file_name = vclimg.create_unique(img_root, vcl_format);

    Json::Value props;
    if (cmd.isMember("properties")) {
        props = cmd["properties"];
    }
    props[ATHENA_IM_PATH_PROP] = file_name;

    // Add Image node
    cmds.push_back(AddNode(grp_id, node_ref, ATHENA_IM_TAG, props));

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

            cmds.push_back(AddEdge(grp_id, -1, src, dst, e_tag, props_link));
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
            cmds.push_back(AddNode(grp_id, col_ref, col_tag, props_col,
                                    constraints, unique));

            // Add edge between collection and image
            cmds.push_back(AddEdge(grp_id, -1, col_ref, node_ref,
                                    ATHENA_COL_EDGE_TAG, props_col));
        }
    }

    return 0;
}

Json::Value AddImage::construct_responses(
    std::vector<pmgdCmdResponse *> &responses,
    const Json::Value &json,
    protobufs::queryMessage &query_res)
{
    Json::Value resp = check_responses(responses);

    Json::Value ret;
    ret[_cmd_name] = resp;
    return ret;
}

//========= FindImage definitions =========

int FindImage::construct_protobuf(
    std::vector<pmgd::protobufs::Command*> &cmds,
    const Json::Value& jsoncmd,
    const std::string& blob,
    int grp_id)
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

    Json::Value arr;
    arr.append(ATHENA_IM_PATH_PROP);
    results["list"] = arr;

    // if (find_img.isMember("collections")) {
    //     Json::Value collections = find_img["collections"];

    //     for (auto col : collections) {
    //         // Do stuff with the collections
    //         // Here we will need and/or etc.
    //     }
    // }

    cmds.push_back(
        QueryNode(grp_id, node_ref, tag, link, constraints, results, unique));

    return 0;
}

Json::Value FindImage::construct_responses(
    std::vector<pmgdCmdResponse *> &responses,
    const Json::Value &json,
    protobufs::queryMessage &query_res)
{
    Json::Value findImage;
    const Json::Value &cmd = json[_cmd_name];

    Json::Value ret;

    bool flag_error = false;

    if (responses.size() == 0) {
        findImage["status"]  = pmgdCmdResponse::Error;
        findImage["info"] = "Not Found!";
        flag_error = true;
        ret[_cmd_name] = findImage;
        return ret;
    }

    for (auto pmgd_res : responses) {

        if (pmgd_res->error_code() != 0) {
            findImage["status"] = pmgd_res->error_code();
            findImage["info"]   = pmgd_res->error_msg();

            flag_error = true;
            break;
        }

        // This list will be the one with the imgPath information
        if (pmgd_res->r_type() != pmgd::protobufs::List) {
            continue;
        }

        findImage = parse_response(pmgd_res);

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
                findImage["status"] = pmgdCmdResponse::Error;
                findImage["info"]   = "VCL Exception";
                flag_error = true;
                break;
            }
        }
    }

    if (!flag_error) {
        findImage["status"] = pmgdCmdResponse::Success;
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
