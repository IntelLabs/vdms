
import json
import pyaml
import sys

from collections import OrderedDict

config_file_name = sys.argv[1]
print(config_file_name)
config_file = open(config_file_name, "r")
config = json.load(config_file)

manager = config["manager"]
data_stores = config["data_stores"]
workers = config["workers"]

output = OrderedDict()
output["version"] = "\'2.0\'"
services = {}

output_environment_workers = ""

for this_data_store in data_stores:
    output_data_store = {}
    output_data_store["container_name"] = "\'" + this_data_store["name"] + "\'"
    output_data_store_ports = []
    output_data_store_ports.append( "\'" + str(this_data_store["port"]) + ":" + str(this_data_store["port"]) + "\'")
    output_data_store["ports"] = output_data_store_ports
    output_data_store_build = {}
    output_data_store_build["context"] = "\'data_store\'"
    output_data_store_build["dockerfile"] = "\'Dockerfile\'"
    output_data_store["build"] = output_data_store_build
    output_data_store_environment = {}
    output_data_store_environment["NETWORK_PORT"] = this_data_store["port"]
    output_data_store["environment"] = output_data_store_environment
    services[this_data_store["name"]] = output_data_store



output_manager = {}
output_manager["container_name"] = "\'" + manager["name"] + "\'"
output_manager_ports = []
output_manager_ports.append( "\'" + str(manager["port"]) + ":" + str(manager["port"]) + "\'")
output_manager["ports"] = output_manager_ports
output_manager_build = {}
output_manager_build["context"] = "\'manager\'"
output_manager_build["dockerfile"] = "\'Dockerfile\'"
output_manager["build"] = output_manager_build
output_manager_environment = {}
output_manager_environment["NETWORK_PORT"] = manager["port"]
#output_manager_environment["WORKERS"] = output_environment_workers
output_manager["environment"] = output_manager_environment
services[manager["name"]] = output_manager

output_worker_dependson = []
for this_worker in workers:
    output_worker = {}
    output_worker_depends_on = []
    output_worker_sources = ""
    output_worker_destinations = ""

    i = 0
    for this_source in this_worker["sources"]:
        output_worker_depends_on.append("\'" + this_source.split(":")[0] + "\'")
        output_worker_sources = output_worker_sources + this_source
        i = i+1
        print(len(this_worker))
        if(i < len(this_worker["sources"]) -1):
            output_worker_sources = output_worker_sources + ","

    for this_destination in this_worker["destinations"]:
        output_worker_depends_on.append("\'" + this_destination.split(":")[0] + "\'")
        output_worker_destinations = output_worker_destinations + this_destination + ","
        
        
    output_worker["depends_on"] = output_worker_depends_on
    output_worker["container_name"] = "\'" + this_worker["name"] + "\'"
    
    output_worker_build = {}
    output_worker_build["context"] = "\'plugins\'"
    output_worker_build["dockerfile"] = "\'Dockerfile\'"
    output_worker["build"] = output_worker_build
    output_worker_environment = {}
    output_worker_environment["SOURCES"] = output_worker_sources
    output_worker_environment["DESTINATIONS"] = output_worker_destinations    
    output_worker["environment"] = output_worker_environment

    services[this_worker["name"]] = output_worker

    
output["services"] = services

print(pyaml.dump(output))

output_file = open("docker-compose.yml", "w")
pyaml.dump(output, output_file)
