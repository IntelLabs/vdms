
import json
import pyaml
import sys

from collections import OrderedDict

config_file_name = sys.argv[1]
print(config_file_name)
config_file = open(config_file_name, "r")
config = json.load(config_file)

manager = config["manager"]
workers = config["workers"]

output = OrderedDict()
output["version"] = "\'2.0\'"
services = {}

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
output_manager["environment"] = output_manager_environment
services[manager["name"]] = output_manager

for this_worker in workers:
    output_worker = {}
    output_worker["container_name"] = "\'" + this_worker["name"] + "\'"
    output_worker_ports = []
    output_worker_ports.append( "\'" + str(this_worker["port"]) + ":" + str(this_worker["port"]) + "\'")
    output_worker["ports"] = output_worker_ports
    output_worker_build = {}
    output_worker_build["context"] = "\'worker\'"
    output_worker_build["dockerfile"] = "\'Dockerfile\'"
    output_worker["build"] = output_worker_build
    output_worker_environment = {}
    output_worker_environment["NETWORK_PORT"] = this_worker["port"]
    output_worker["environment"] = output_worker_environment
    services[this_worker["name"]] = output_worker

output["services"] = services
print(pyaml.dump(output))

output_file = open("docker_compose.yml", "w")
pyaml.dump(output, output_file)