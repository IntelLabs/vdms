import os
import json
import zmq
import sys
import importlib.util

DEBUG_MODE=True

tmp_dir_path = None
functions_dir_path = None

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

def setup(functions_path, settings_path, tmp_path):
    global tmp_dir_path
    global functions_dir_path
    if DEBUG_MODE:
        print("Setup for udf_local", file=sys.stderr)
        print("functions_path:", functions_path, file=sys.stderr)
        print("settings_path:", settings_path, file=sys.stderr)
        print("tmp_path:", tmp_path, file=sys.stderr)
    if functions_path is None:
        functions_path = os.path.join(os.getcwd(), "functions")
        print("Warning: Using functions dir:", functions_path, " as default.")

    if not os.path.exists(functions_path):
        raise Exception(f"{functions_path} path is invalid")

    if settings_path is None:
        settings_path = os.path.join(os.getcwd(), "settings.json")
        print("Warning: Using settings dir:", settings_path, " as default.")

    if not os.path.exists(settings_path):
        raise Exception(f"{settings_path} path is invalid")

    if tmp_path is None:
        tmp_path = os.path.join(os.getcwd(), "tmp")
        print("Warning: Using temporary dir:", tmp_path, " as default.")

    if not os.path.exists(tmp_path):
        raise Exception(f"{tmp_path}: path to temporary dir is invalid")
    
    # Set path to temporary dir
    tmp_dir_path = tmp_path

    # Set path to functions dir
    functions_dir_path = functions_path

    for entry in os.scandir(functions_path):
        if entry.is_file() and entry.path.endswith(".py"):
		    if DEBUG_MODE:
                print("Checking:", entry.name)
            module_name = entry.name[:-3]
            if DEBUG_MODE:
			    print("Module:", module_name)

            # Import the module from the given path
            module = import_module_from_path(module_name, entry)
            if module is None:
                raise Exception("setup() error: module '" + entry + "' could not be loaded")
            globals()[module_name] = module

    with open(settings_path, "r") as settings_file:
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
            if DEBUG_MODE:
                print("Module called:", udf, file=sys.stderr)

            response, _ = udf.run(settings, input_params["ipfile"], input_params, tmp_dir_path, functions_dir_path)

            # print(i, response)
            socket.send_string(response)
            i += 1
        except Exception as e:
		    if DEBUG_MODE:
                print(e.with_traceback(None), file=sys.stderr)
                print("Exception in Flip:", str(e), file=sys.stderr)
            socket.send_string("An error occurred while running the operation.")
            break


if __name__ == "__main__":
    if sys.argv[1] == None:
        print(
            "Warning: Path to the functions directory is missing\nBy default the path will be the current directory"
        )
        print("Correct Usage: python3 udf_local.py [functions_path] [settings_path] [tmp_path]")
    elif sys.argv[2] == None:
        print(
            "Warning: Path to the settings directory is missing\nBy default the path will be the current directory"
        )
        print("Correct Usage: python3 udf_local.py [functions_path] [settings_path] [tmp_path]")
    elif sys.argv[3] == None:
        print(
            "Warning: Path to the temporary directory is missing\nBy default the path will be the current directory"
        )
        print("Correct Usage: python3 udf_local.py [functions_path] [settings_path] [tmp_path]")
    setup(sys.argv[1], sys.argv[2], sys.argv[3])
