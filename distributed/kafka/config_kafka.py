import os
import json
#open the file
config_file = open("config-vdms.json", "r")


#for every line in the file
#if the line containes stub_start and stub_end
   #then replace with the envionment_variable that has stripped the first two and last two elements
   #for a = 2 to len-1 aand append with "_"




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
