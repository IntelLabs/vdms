import os
import json
#open the file
config_file = open("config-vdms.json", "r")
config = json.load(config_file)
config_file.close()

try:
    new_port = os.environ["NETWORK_PORT"]
    config_file = open("config-vdms.json", "w")
    config["port"] = int(new_port)
    json.dump(config, config_file)
    config_file.close()
except KeyError:
    pass


#write the file back
