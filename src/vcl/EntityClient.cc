#include <grpcpp/grpcpp.h>
#include "entity.grpc.pb.h"
#include <memory.h>
#include <iostream>
#include <fstream>
#include <cmath>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/writer.h>
#include <vector>
#include <numeric>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using entity::Operator;
using entity::Entity;

class EntityClient{
    public:
        EntityClient(std::shared_ptr<Channel> channel) : _stub{Operator::NewStub(channel)} {}

        Json::Value Operate(std::string &entity, std::string &options, std::string &filePath){
            Entity request, response;

            request.set_entity(entity);
            request.set_options(options);

            ClientContext context;

            Status status = _stub->Operate(&context, request, &response);

            Json::Value jsonValue;

            if (status.ok()) {
                std::ofstream file(filePath.c_str(), std::ios::binary);
                if (!file) {
                    std::cerr << "Error creating file" << std::endl;
                    return jsonValue;
                }
                file.write((char*) response.entity().c_str(), response.entity().size());
                file.close();

                std::string jsonString(response.options().c_str());
                
                Json::CharReaderBuilder builder;
                std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
                std::string errors;

                bool parsingSuccessful = reader->parse(
                    jsonString.c_str(),
                    jsonString.c_str() + jsonString.length(),
                    &jsonValue,
                    &errors
                );

                if (!parsingSuccessful) {
                    std::cout << "Failed to parse JSON: " << errors << std::endl;                    
                }

            } else {
                std::cout << status.error_code() << ": " << status.error_message()
                            << std::endl;
                // return "RPC failed";
            }

            return jsonValue;
        }
    private:
        std::unique_ptr<Operator::Stub> _stub;

};