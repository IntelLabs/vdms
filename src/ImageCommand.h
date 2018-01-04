#pragma once
#include <string>
#include <mutex>
#include <vector>
#include "VCL.h"

#include "PMGDTransaction.h"
#include "RSCommand.h"
#include "ExceptionsCommand.h"

namespace athena {

// Helper classes for handling various JSON commands.

    class ImageCommand: public RSCommand
    {

    protected:
        void run_operations(VCL::Image& vclimg, const Json::Value& op);

    public:

        ImageCommand(const std::string &cmd_name);

        virtual int construct_protobuf(PMGDTransaction& tx,
                               const Json::Value& root,
                               const std::string& blob,
                               int grp_id,
                               Json::Value& error) = 0;

        virtual bool need_blob() { return false; }

        virtual Json::Value construct_responses(
                Json::Value &json_responses,
                const Json::Value &json,
                protobufs::queryMessage &response) = 0;
    };

    class AddImage: public ImageCommand
    {
        const std::string DEFAULT_TDB_PATH = "./tdb_database";
        const std::string DEFAULT_PNG_PATH = "./png_database";
        const std::string DEFAULT_JPG_PATH = "./jpg_database";

        std::string _storage_tdb;
        std::string _storage_png;
        std::string _storage_jpg;

    public:
        AddImage();

        int construct_protobuf(PMGDTransaction& tx,
                               const Json::Value& root,
                               const std::string& blob,
                               int grp_id,
                               Json::Value& error);

        bool need_blob() { return true; }

        Json::Value construct_responses(
                Json::Value &json_responses,
                const Json::Value &json,
                protobufs::queryMessage &response);
    };

    class FindImage: public ImageCommand
    {
    public:
        FindImage();
        int construct_protobuf(PMGDTransaction& tx,
                               const Json::Value& root,
                               const std::string& blob,
                               int grp_id,
                               Json::Value& error);

        Json::Value construct_responses(
                Json::Value &json_responses,
                const Json::Value &json,
                protobufs::queryMessage &response);
    };

}; // namespace athena
