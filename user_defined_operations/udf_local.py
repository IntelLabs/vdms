import os
import json
import zmq

for entry in os.scandir("functions"):
    if entry.is_file():
        string = f"from functions import {entry.name}"[:-3]
        exec(string)

with open("settings.json", "r") as settings_file:
    settings_data = settings_file.read()

# parse file
settings = json.loads(settings_data)

context = zmq.Context()
socket = context.socket(zmq.REP)
socket.bind("tcp://*:" + str(settings["port"]))

# print(globals())
i = 0
print("Started Listening...")
while True:
    message = socket.recv()

    try:
        print("Received {}".format(message))

        message_received = message.decode("utf-8")
        input_params = json.loads(message_received)

        udf = globals()[settings["functions"][input_params["id"]]]

        t, opfile = udf.run(settings, input_params["ipfile"], input_params)

        print(t, i, opfile)
        socket.send_string(opfile)
        i += 1
    except Exception as e:
        print(e.with_traceback(None))
        socket.send_string("An error occurred while running the operation.")
        break
