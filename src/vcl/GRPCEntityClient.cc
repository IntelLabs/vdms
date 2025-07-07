#include "vcl/GRPCEntityClient.h"

#include <fstream>
#include <iostream>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/writer.h>

#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/support/channel_arguments.h>

using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::Status;
using grpc::ChannelArguments;
using entity::Entity;
using entity::Operator;

int MAX_SIZE_BYTES = 500*1024*1024;

struct GRPCEntityClient::AsyncCall {
    Entity response;
    ClientContext context;
    Status status;
    std::unique_ptr<ClientAsyncResponseReader<Entity>> reader;
    std::string entity_id;
    GRPCEntityClient* parent;
};

GRPCEntityClient::GRPCEntityClient(std::string url)
    : url_(std::move(url)) {
    unsigned int concurrency = std::thread::hardware_concurrency();
    if (concurrency == 0) concurrency = 8;
    max_concurrent_tasks_ = std::min(64u, concurrency * 2);
}

void GRPCEntityClient::InitStub() {
    if (!stub_) {
        grpc::ChannelArguments args;
        args.SetMaxReceiveMessageSize(MAX_SIZE_BYTES);
        args.SetMaxSendMessageSize(MAX_SIZE_BYTES);
        stub_ = Operator::NewStub(
            grpc::CreateCustomChannel(url_, grpc::InsecureChannelCredentials(), args)
        );
        std::cerr << "Stub initialized for URL: " << url_ << "\n";
    }
}

void GRPCEntityClient::ProcessEntities(const std::map<std::string, std::string>& input_paths,
                                       const std::map<std::string, std::string>& output_paths,
                                       const std::map<std::string, std::string>& input_metadata,
                                       std::map<std::string, std::string>& output_metadata,
                                       bool& success) {
    output_metadata_ = &output_metadata;
    output_paths_ = &output_paths;
    input_metadata_ = &input_metadata;
    success_ = &success;

    for (const auto& [entity_id, input_path] : input_paths) {
        WaitForSlot();
        SendRequest(entity_id, input_path);
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        all_done_.wait(lock, [this] { return in_flight_ == 0; });
    }

    cq_.Shutdown();
    if (worker_.joinable()) {
        worker_.join();
    }
}

void GRPCEntityClient::SendRequest(const std::string& entity_id, const std::string& input_path) {
    
    InitStub();
    auto* call = new AsyncCall;
    call->entity_id = entity_id;
    call->parent = this;

    Entity request;
    std::string entity_data;
    if (!ReadFile(input_path, entity_data)) {
        std::cerr << "Failed to read " << input_path << "\n";
        *success_ = false;
        delete call;
        return;
    }

    request.set_entity(entity_data);

    auto it = input_metadata_->find(entity_id);
    std::string json = (it != input_metadata_->end()) ? it->second : "{}";
    request.set_options(json);

    // Set timeout
    // call->context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(30));

    call->reader = stub_->AsyncOperate(&call->context, request, &cq_);
    call->reader->Finish(&call->response, &call->status, (void*)call);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        ++in_flight_;
    }
}

bool GRPCEntityClient::ReadFile(const std::string& path, std::string& out) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "Cannot open file: " << path << "\n";
        return false;
    }
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    if (size == 0) {
        *success_ = false;
        std::cerr << "File is empty: " << path << "\n";
        return false;
    }
    file.seekg(0);
    out.resize(size);
    file.read(&out[0], size);
    if (!file) {
        *success_ = false;
        std::cerr << "Failed to read full file: " << path << "\n";
        return false;
    }
    return true;
}

bool GRPCEntityClient::WriteFile(const std::string& path, const std::string& data) {
    std::ofstream file(path, std::ios::binary);
    if (!file){
        *success_ = false;
        return false;
    } 
    file.write(data.data(), data.size());
    return file.good();
}

void GRPCEntityClient::WaitForSlot() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!worker_started_) {
        worker_ = std::thread([this]() { HandleRpcs(); });
        worker_started_ = true;
    }
    cond_.wait(lock, [this]() { return in_flight_ < max_concurrent_tasks_; });
}

void GRPCEntityClient::HandleRpcs() {
    void* tag;
    bool ok = false;
    while (cq_.Next(&tag, &ok)) {
        auto* call = static_cast<AsyncCall*>(tag);
        if (ok && call->status.ok()) {
            const std::string& out_data = call->response.entity();
            const std::string& out_json = call->response.options();

            std::string output_path = output_paths_->at(call->entity_id);
            if (!WriteFile(output_path, out_data)) {
                *success_ = false;
                std::cerr << "Failed to write entity: " << output_path << "\n";
            }

            (*output_metadata_)[call->entity_id] = out_json;
        } else {
            *success_ = false;
            std::cerr << "RPC failed or connection error for: " << call->entity_id
                      << " - status: " << call->status.error_message() << " " << int(*success_) << "\n";
        }

        {
            std::lock_guard<std::mutex> lock(mutex_);
            --in_flight_;
            cond_.notify_one();
            if (in_flight_ == 0) {
                all_done_.notify_one();
            }
        }

        delete call;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (in_flight_ == 0) {
            all_done_.notify_one();
        }
    }
}
