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

        void Operate(std::string &entity, std::string &options){
            Entity request, response;

            request.set_entity(entity);
            request.set_options(options);

            ClientContext context;

            Status status = _stub->Operate(&context, request, &response);

            if (status.ok()) {
                FILE *out = fopen("cpp.mp4","w");
                fwrite((char*) response.entity().c_str(), 1, response.entity().length(), out);
                std::cout<< "Response " << response.options() << std::endl;

            } else {
                std::cout << status.error_code() << ": " << status.error_message()
                            << std::endl;
                // return "RPC failed";
            }
        }
    private:
        std::unique_ptr<Operator::Stub> _stub;

};