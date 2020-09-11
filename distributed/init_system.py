
import json
import pyaml
import sys

config_file_name = sys.argv[1]
print(config_file_name)
config_file = open(config_file_name, "r")
config = json.load(config_file)
controller = config["controller"]
workers = config["workers"]

output_yaml_dict = {}
output_yaml_dict["version"] = '3.8'
services = []
output_controller = {}

print(pyaml.dump(workers))