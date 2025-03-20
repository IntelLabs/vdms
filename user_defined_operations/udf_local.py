import os
import json
import zmq
import sys
import importlib.util


# Function to dynamically import a module given its full path
def import_module_from_path(module_name, path):
    try:
        # Create a module spec from the given path
        spec = importlib.util.spec_from_file_location(module_name, path)

        # Load the module from the created spec
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        return module
    except Exception as e:
        print("import_module_from_path() failed:", str(e), file=sys.stderr)
        return None


def setup():
    # Get the real directory where this Python file is
    currentDir = os.path.realpath(os.path.dirname(__file__))

    functions_path = os.path.join(currentDir, "functions")
    if not os.path.exists(functions_path):
        raise Exception(f"{functions_path} path is invalid")

    settings_path = os.path.join(currentDir, "settings.json")
    if not os.path.exists(settings_path):
        raise Exception(f"{settings_path} path is invalid")

    for entry in os.scandir(functions_path):
        if entry.is_file() and entry.path.endswith(".py"):
            module_name = entry.name[:-3]

            # Import the module from the given path
            module = import_module_from_path(module_name, entry)
            if module is None:
                raise Exception(
                    "setup() error: module '" + entry + "' could not be loaded"
                )
            globals()[module_name] = module
    with open(settings_path, "r") as settings_file:
        settings_data = settings_file.read()

    # parse file
    settings = json.loads(settings_data)

    context = zmq.Context()
    socket = context.socket(zmq.REP)
    socket.bind("tcp://*:" + str(settings["port"]))

    i = 0
    print("Started Listening...")
    while True:
        message = socket.recv()

        try:
            print("Received {}".format(message))

            message_received = message.decode("utf-8")
            input_params = json.loads(message_received)

            if "functions" not in settings:
                raise Exception("functions value was not found in settings")
            settings_value = settings["functions"]

            if "id" not in input_params:
                raise Exception("id value was not found in input_params")
            id_value = input_params["id"]

            if id_value not in settings_value:
                raise Exception(f"{id_value} value was not found in {settings_value}")
            udf_key = settings_value[id_value]

            if udf_key not in globals():
                raise Exception(f"{udf_key} value was not found in globals()")

            udf = globals()[udf_key]

            response, _ = udf.run(
                settings,
                input_params["ipfile"],
                input_params,
            )

            socket.send_string(response)
            i += 1
        except Exception:
            socket.send_string("An error occurred while running the operation.")
            break


def main():
    setup()


if __name__ == "__main__":
    main()
