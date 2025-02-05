#include <iostream>
#include <string>
#include<vector>

namespace kubernetes {
    // Helper classe for handling Kubernetes API calls
    // pod_creator method will help populate the fields required to create a pod within the cluster with a PodNAme in the argument
    // service_creator method will help populate fields specific to crearion of a service with the AppSelector and ServiceName in the arguments
    class KubeHelper {
    public:
        //define method to create a pod
        int pod_creator(char *PodName, std::string worker_node_name);
        int service_creator(char *ServiceName,char *AppSelector);
        std::vector<std::string> get_workernode();
        std::string k8s_objects_creator(char* appname,std::string node_name);
        std::string query_scheduler(std::string mediaType);
        static inline int query_counter=0;
    };
} // namespace kubernetes
