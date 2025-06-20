#ifndef GRPC_ENTITY_CLIENT_H
#define GRPC_ENTITY_CLIENT_H

#include <grpcpp/grpcpp.h>
#include <grpcpp/alarm.h>
#include "entity.grpc.pb.h"

#include <string>
#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>

class GRPCEntityClient {
public:
    explicit GRPCEntityClient(std::shared_ptr<grpc::Channel> channel);

    void ProcessEntities(const std::map<std::string, std::string>& input_paths,
                         const std::map<std::string, std::string>& output_paths,
                         const std::map<std::string, std::string>& input_metadata,
                         std::map<std::string, std::string>& output_metadata);

private:
    struct AsyncCall;

    void SendRequest(const std::string& entity_id, const std::string& input_path);
    bool ReadFile(const std::string& path, std::string& out);
    bool WriteFile(const std::string& path, const std::string& data);
    void WaitForSlot();
    void HandleRpcs();

    std::unique_ptr<entity::Operator::Stub> stub_;
    grpc::CompletionQueue cq_;
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

#endif  // GRPC_ENTITY_CLIENT_H