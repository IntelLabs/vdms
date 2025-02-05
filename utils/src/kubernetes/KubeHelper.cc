#include "KubeHelper.h"
#include <cstdlib>
#include <iostream>
#include <jsoncpp/json/json.h>
#include <fstream>
#include <chrono>
#include <thread>


extern "C"{
#include </usr/local/include/kubernetes/config/incluster_config.h>
#include </usr/local/include/kubernetes/include/apiClient.h>
#include </usr/local/include/kubernetes/api/CoreV1API.h>
#include </usr/local/include/kubernetes/config/kube_config.h>
}

using namespace kubernetes;
using namespace std::chrono;
using namespace std::this_thread;
//This function will load the necessary authentication configs for interating with the kube-api-server
// It will populate the necessary fields and then create a pod
int KubeHelper::pod_creator(char* PodName,std::string worker_node_name){
    char *basePath = NULL;
    sslConfig_t *sslConfig = NULL;
    list_t *apiKeys = NULL;
    int rc = load_incluster_config(&basePath, &sslConfig, &apiKeys);
        if (rc != 0) {
            printf("Cannot load kubernetes configuration in cluster.\n");
            return -1;
        }
    apiClient_t *apiClient = apiClient_create_with_base_path(basePath, sslConfig, apiKeys);
        if (!apiClient) {
            printf("Cannot create a kubernetes client.\n");
            return -1;
        }else{
            v1_pod_t *podinfo = (v1_pod_t*)calloc(1, sizeof(v1_pod_t));
            list_t *conportList = list_createList();
            podinfo->api_version = strdup("v1");
            podinfo->kind = strdup("Pod");
            podinfo->spec = (v1_pod_spec_t*)calloc(1, sizeof(v1_pod_spec_t));
            podinfo->metadata = (v1_object_meta_t*)calloc(1, sizeof(v1_object_meta_t));
            list_t *labelList = list_createList();
            keyValuePair_t *label = keyValuePair_create(strdup("app"), PodName);
            list_addElement(labelList,label);
            const char* node = worker_node_name.c_str();
            keyValuePair_t *nodeSelect = keyValuePair_create(strdup("nodeSelector"), strdup(node));
            list_t *node_select_list = list_createList();
            list_addElement(node_select_list, nodeSelect);
            /* set pod name */
            podinfo->metadata->name = PodName;
            podinfo->metadata->labels = labelList;
            v1_container_port_t *containerPort = (v1_container_port_t*)calloc(1, sizeof(v1_container_port_t));
            containerPort->container_port = 5050;
            /* set containers for pod */
            list_t *containerlist = list_createList();
            v1_container_t *con = (v1_container_t*)calloc(1, sizeof(v1_container_t));
            con->name = strdup("my-container-fnfdnvnoimdk");
            con->image = strdup("localhost:5000/remote-udf-1");
            con->image_pull_policy = strdup("Always");
            con->ports = conportList;
            list_addElement(containerlist, con);
            podinfo->spec->containers = containerlist;
            podinfo->spec->node_selector = node_select_list;
            char* ns = "default";

            /* call API in libkubernetes to create pod */
            v1_pod_t *apod = CoreV1API_createNamespacedPod(apiClient,ns, podinfo, NULL, NULL, NULL, NULL);
                if(apiClient->response_code != 200 && apiClient->response_code != 201) {
                        fprintf(stderr, "Failed to create service: %ld\n", apiClient->response_code);
                        free(podinfo);
                        free(apod);
                        free_client_config(basePath, sslConfig, apiKeys);
                        basePath = NULL;
                        sslConfig = NULL;
                        apiKeys = NULL;
                        free(apiClient);
                        apiClient_unsetupGlobalEnv();
                        return -1;
                    }else{
                        printf("Service created successfully\n");
                        free(podinfo);
                        free(apod);
                        free_client_config(basePath, sslConfig, apiKeys);
                        basePath = NULL;
                        sslConfig = NULL;
                        apiKeys = NULL;
                        free(apiClient);
                        apiClient_unsetupGlobalEnv();
                    return 0;}
        }        
    return -1;   
}
//This function will load the necessary authentication configs for interating with the kube-api-server
// It will populate the necessary fields and then create a service in the cluster
int KubeHelper::service_creator(char * ServiceName, char *AppSelector){
    char *basePath = NULL;
    sslConfig_t *sslConfig = NULL;
    list_t *apiKeys = NULL;

    int rc = load_incluster_config(&basePath, &sslConfig, &apiKeys);

    if (rc != 0) {
        printf("Cannot load kubernetes configuration in cluster.\n");
        return -1;
    }
    apiClient_t *apiClient = apiClient_create_with_base_path(basePath, sslConfig, apiKeys);
    if (!apiClient) {
        printf("Cannot create a kubernetes client.\n");
        return -1;
    }
        if(apiClient){
            v1_service_port_t *servicePort = (v1_service_port_t*)calloc(1, sizeof(v1_service_port_t));
                if (!servicePort) {
                    fprintf(stderr, "Memory allocation for servicePort failed\n");
                    return -1;}      
            int prt = 5050;
            servicePort->port = prt;
            // Add the ServicePort to a list
            list_t *servicePortList = list_createList();
                if (!servicePortList) {
                    fprintf(stderr, "Memory allocation for servicePortList failed\n");
                    free(servicePort);
                    return -1;}
        list_addElement(servicePortList, servicePort);
        // Create a ServiceSpec
        v1_service_spec_t *serviceSpec = (v1_service_spec_t*)calloc(1, sizeof(v1_service_spec_t));
            if (!serviceSpec) {
                fprintf(stderr, "Memory allocation for serviceSpec failed\n");
                free(servicePort);
                list_freeList(servicePortList);
                return -1;}
        keyValuePair_t *selectorPair = keyValuePair_create(strdup("app"), strdup(AppSelector));
        list_t *selectorlist = list_createList();
        list_addElement(selectorlist, selectorPair);
        serviceSpec->selector = selectorlist;
        serviceSpec->ports = servicePortList;
            if (!serviceSpec->selector) {
                fprintf(stderr, "Memory allocation for serviceSpec->selector failed\n");
                free(servicePort);
                list_freeList(servicePortList);
                free(serviceSpec);
                return -1;}

        // Create a Service
        v1_service_t *service = (v1_service_t*)calloc(1, sizeof(v1_service_t));
            if (!service) {
                fprintf(stderr, "Memory allocation for service failed\n");}
        service->api_version = strdup("v1");
        service->kind = strdup("Service");
        service->metadata = (v1_object_meta_t*)calloc(1, sizeof(v1_object_meta_t));
        service->metadata->name = strdup(ServiceName);
        service->spec = serviceSpec;
        char *response = NULL;
        // Create the service in the specified namespace
        CoreV1API_createNamespacedService(apiClient, "default", service, NULL, NULL, NULL,NULL);
        // Check the response code
            if(apiClient->response_code != 200 && apiClient->response_code != 201) {
                fprintf(stderr, "Failed to create service: %ld\n", apiClient->response_code);
                v1_service_free(service);
                return -1;
            }else{
                printf("Service created successfully\n");
                v1_service_free(service);
                return 0;}
        }
    return -1;
}

std::vector<std::string> KubeHelper::get_workernode(){
    std::string filename = "/etc/config/kubeConfig.json";
    std::ifstream fileStream(filename);
    if (!fileStream.is_open()) {
        std::cerr << "Failed to open " << filename << std::endl;  
    }
    Json::Value root;
    Json::CharReaderBuilder builder;
    JSONCPP_STRING errs;
    if (!Json::parseFromStream(builder, fileStream, &root, &errs)) {
        std::cerr << "Failed to parse JSON: " << errs << std::endl;
    }
    fileStream.close();
    std::vector<std::string> node_names;
    if (root.isMember("WorkerNodeDetail") && root["WorkerNodeDetail"].isArray()) {
        const Json::Value workerNodeDetail = root["WorkerNodeDetail"];
        for (const auto& nodeDetail : workerNodeDetail) {
            for (Json::ValueConstIterator it = nodeDetail.begin(); it != nodeDetail.end(); ++it) {
                std::string nodeName = it.key().asString();
                std::string nodeIP = it->asString();
                std::cout << "Node Name: " << nodeName << ", IP: " << nodeIP << std::endl;
                node_names.push_back(nodeName);
            }
        }
    }
    else{
        std::cout<<"Can not parse the kubernetes config for VDMS";
    }
    return node_names;
}

// Orchestration Creator
std::string KubeHelper::k8s_objects_creator(char* appname,std::string node_name){
    KubeHelper k8s_object;
    int status_pod = k8s_object.pod_creator(appname,node_name);
    sleep_for(nanoseconds(10));
    sleep_until(system_clock::now() + seconds(5));
    if (status_pod!=-1){
        std::string app_str(appname);
        std::string svc_name_str = app_str+"svc";
        char* svc_name = &svc_name_str[0];
        int status_svc = k8s_object.service_creator(svc_name,appname);
        sleep_for(nanoseconds(10));
        sleep_until(system_clock::now() + seconds(2));
        if(status_svc!=-1){
            std::string url_final = svc_name_str +":5050/image";
            return url_final;}}
}


// Scheduler
std::string KubeHelper::query_scheduler(std::string mediaType){
    // Below logic creates parameters for creating new pods in cluster with help of kube-api-server
    std::string url_final;
    KubeHelper k8s_object;

    static std::vector<std::string> worker_nodes_list = k8s_object.get_workernode(); 
    static int num_worker_node = worker_nodes_list.size();
    if((KubeHelper::query_counter)>=0 && (KubeHelper::query_counter)<num_worker_node){
        std::string app = "rudf";
        std::string num = std::to_string(KubeHelper::query_counter);
        app = app+num;
        char* appname = &app[0];
        std::string node_name = worker_nodes_list[KubeHelper::query_counter];
        std::string url_final = k8s_object.k8s_objects_creator(appname, node_name);
        (KubeHelper::query_counter)++;
        return url_final;
    }
    else{
        //the new load balancer - Round Robin 
        int div = (KubeHelper::query_counter)%num_worker_node;
        std::string sv = "rudf";
        std::string num = std::to_string(div);
        sv = sv+num+"svc";
        url_final = sv + ":5050/" + mediaType;
        (KubeHelper::query_counter)++;
        return url_final;
    }
}