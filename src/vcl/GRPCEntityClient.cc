#include <grpcpp/grpcpp.h>
#include <grpcpp/alarm.h>
#include "entity.grpc.pb.h"

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/writer.h>
#include <chrono>

using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;
using entity::Entity;
using entity::Operator;

class GRPCEntityClient {
public:
    GRPCEntityClient(std::shared_ptr<Channel> channel)
        : stub_(Operator::NewStub(channel)) {
            unsigned int concurrency = std::thread::hardware_concurrency();
            if (concurrency == 0) concurrency = 8;
            max_concurrent_tasks_ = std::min(64u, concurrency * 2);
        }   

    void ProcessEntities(const std::map<std::string, std::string>& input_paths,
                       const std::map<std::string, std::string>& output_paths,
                       const std::map<std::string, std::string>& input_metadata,
                       std::map<std::string, std::string>& output_metadata) {
        output_metadata_ = &output_metadata;
        output_paths_ = &output_paths;
        input_metadata_ = &input_metadata;

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

private:
    struct AsyncCall {
        Entity response;
        ClientContext context;
        Status status;
        std::unique_ptr<ClientAsyncResponseReader<Entity>> reader;
        std::string entity_id;
        GRPCEntityClient* parent;
    };

    void SendRequest(const std::string& entity_id, const std::string& input_path) {
        auto* call = new AsyncCall;
        call->entity_id = entity_id;
        call->parent = this;

        Entity request;
        std::string entity_data;
        if (!ReadFile(input_path, entity_data)) {
            std::cerr << "Failed to read " << input_path << "\n";
            delete call;
            return;
        }

        request.set_entity(entity_data);

        auto it = input_metadata_->find(entity_id);
        std::string json = (it != input_metadata_->end()) ? it->second : "{}";
        request.set_options(json);

        call->reader = stub_->AsyncOperate(&call->context, request, &cq_);
        call->reader->Finish(&call->response, &call->status, (void*)call);

        {
            std::lock_guard<std::mutex> lock(mutex_);
            ++in_flight_;
        }
    }

    bool ReadFile(const std::string& path, std::string& out) {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            std::cerr << "Cannot open file: " << path << "\n";
            return false;
        }
        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        if (size == 0) {
            std::cerr << "File is empty: " << path << "\n";
            return false;
        }
        file.seekg(0);
        out.resize(size);
        file.read(&out[0], size);
        if (!file) {
            std::cerr << "Failed to read full file: " << path << "\n";
            return false;
        }
        return true;
    }

    bool WriteFile(const std::string& path, const std::string& data) {
        std::ofstream file(path, std::ios::binary);
        if (!file) return false;
        file.write(data.data(), data.size());
        return file.good();
    }

    void WaitForSlot() {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!worker_started_) {
            worker_ = std::thread([this]() { HandleRpcs(); });
            worker_started_ = true;
        }
        cond_.wait(lock, [this]() { return in_flight_ < max_concurrent_tasks_; });
    }

    void HandleRpcs() {
        void* tag;
        bool ok = false;
        while (cq_.Next(&tag, &ok)) {
            auto* call = static_cast<AsyncCall*>(tag);
            if (ok && call->status.ok()) {
                const std::string& out_data = call->response.entity();
                const std::string& out_json = call->response.options();

                std::string output_path = output_paths_->at(call->entity_id);
                if (!WriteFile(output_path, out_data)) {
                    std::cerr << "Failed to write entity: " << output_path << "\n";
                } 

                (*output_metadata_)[call->entity_id] = out_json;
            } else {
                std::cerr << "RPC failed or connection error for: " << call->entity_id 
                      << " - status: " << call->status.error_message() << "\n";
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

    std::unique_ptr<Operator::Stub> stub_;
    CompletionQueue cq_;
    std::thread worker_;
    bool worker_started_ = false;

    const std::map<std::string, std::string>* output_paths_;
    const std::map<std::string, std::string>* input_metadata_;
    std::map<std::string, std::string>* output_metadata_;

    std::mutex mutex_;
    std::condition_variable cond_;
    std::condition_variable all_done_;
    int in_flight_ = 0;
    int max_concurrent_tasks_;
};