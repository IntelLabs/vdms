from flask import Flask, request, jsonify, send_file, after_this_request
import cv2
import numpy as np
import json
from datetime import datetime, timezone
import os
import sys
from collections import defaultdict, deque
import skvideo.io
import imutils
import uuid
from zipfile import ZipFile, is_zipfile
import importlib.util

DEBUG_MODE = True

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
      print("import_module_from_path() failed:", str(e))
      return None

def setup(functions_path,tmp_path):
    global tmp_dir_path
    global functions_dir_path
	if DEBUG_MODE:
        print("udf_server Calling to setup", file=sys.stderr)
        print("udf_server tmp_path:", tmp_path, file=sys.stderr)
        print("udf_server functions_path:", functions_path, file=sys.stderr)
    if functions_path is None:
        functions_path = os.path.join(os.getcwd(), "functions")
        print("Warning: Using functions dir:", functions_path, " as default.")

    if not os.path.exists(functions_path):
        raise Exception(f"{functions_path}: path to functions dir is invalid")
    
    if tmp_path is None:
        tmp_path = os.path.join(os.getcwd(), "tmp")
        print("Warning: Using temporary dir:", tmp_path, " as default.")

    if not os.path.exists(tmp_path):
        raise Exception(f"{tmp_path}: path to temporary dir is invalid")
    
    # Set path to temporary dir
    tmp_dir_path = tmp_path

    # Set path to functions dir
    functions_dir_path = functions_path

    if DEBUG_MODE:
        print("Searching functions in", functions_path)
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

app = Flask(__name__)

def get_current_timestamp():
    dt = datetime.now(timezone.utc)

    utc_time = dt.replace(tzinfo=timezone.utc)
    utc_timestamp = utc_time.timestamp()

    return utc_timestamp


@app.route("/hello", methods=["GET"])
def hello():
    return jsonify({"response": "true"})


@app.route("/image", methods=["POST"])
def image_api():
    json_data = json.loads(request.form["jsonData"])
    image_data = request.files["imageData"]

    format = json_data["format"] if "format" in json_data else "jpg"

    tmpfile = secure_filename(os.path.join(tmp_dir_path,"tmpfile" + uuid.uuid1().hex + "." + str(format)))

    image_data.save(tmpfile)

    r_img, r_meta = "", ""

    udf = globals()[json_data["id"]]
    if "ingestion" in json_data:
        r_img, r_meta = udf.run(tmpfile, format, json_data, tmp_dir_path, functions_dir_path)
    else:
        r_img, _ = udf.run(tmpfile, format, json_data, tmp_dir_path, functions_dir_path)

    return_string = cv2.imencode("." + str(format), r_img)[1].tostring()

    if r_meta != "":
        return_string += ":metadata:".encode("utf-8")
        return_string += r_meta.encode("utf-8")

    os.remove(tmpfile)
    return return_string


@app.route("/video", methods=["POST"])
def video_api():
    json_data = json.loads(request.form["jsonData"])
    video_data = request.files["videoData"]
    format = json_data["format"] if "format" in json_data else "mp4"

    tmpfile = secure_filename(os.path.join(tmp_dir_path, "tmpfile" + uuid.uuid1().hex + "." + str(format)))
    video_data.save(tmpfile)

    video_file, metadata_file = "", ""

    udf = globals()[json_data["id"]]
    if "ingestion" in json_data:
	    if DEBUG_MODE:
            print("Using ingestion in:", json_data["id"], file=sys.stderr)
        video_file, metadata_file = udf.run(tmpfile, format, json_data, tmp_dir_path, functions_dir_path)
    else:
	    if DEBUG_MODE:
            print("Not using ingestion in:", json_data["id"], file=sys.stderr)
         # TODO: why run in Metadata file returns a tuple and here we are ignoring that value?
        # That is causing the returned tuple is being stored in video_file causing an issue later
        video_file, metadata_file = udf.run(tmpfile, format, json_data, tmp_dir_path, functions_dir_path)

    response_file = os.path.join(tmp_dir_path, "tmpfile" + uuid.uuid1().hex + ".zip")
    if DEBUG_MODE:
        print("video_file:", video_file, file=sys.stderr)

    try:

        with ZipFile(response_file, "w") as zip_object:
            zip_object.write(video_file, os.path.basename(video_file))
            if metadata_file is not None and metadata_file != "":
			    if DEBUG_MODE:
                    print("metadata_file:", metadata_file, file=sys.stderr)
                zip_object.write(metadata_file, os.path.basename(metadata_file))
            zip_object.close()
            if not is_zipfile(response_file):
                raise Exception("response_file is invalid: " + response_file)
    except Exception as e:
        error_message = f"Exception: {str(e)}"
		if DEBUG_MODE:
            print(error_message, file=sys.stderr)
        return error_message, 500

    if DEBUG_MODE:
        print("udf_server tmpfile:", tmpfile, file=sys.stderr)
		print("udf_server response_file:", response_file, file=sys.stderr)

    @after_this_request
    def remove_tempfile(response):
        try:
            os.remove(tmpfile)
            os.remove(response_file)
            os.remove(video_file)
            os.remove(metadata_file)
        except Exception as e:
            print("Some files cannot be deleted or are not present:", str(e), file=sys.stderr)
        return response

    try:
        return send_file(response_file, as_attachment=True, download_name=os.path.basename(response_file))
    except Exception as e:
	    if DEBUG_MODE:
            print(Error in file read:", str(e), file=sys.stderr)
        return "Error in file read"


@app.errorhandler(400)
def handle_bad_request(e):
    response = e.get_response()
    response.data = json.dumps(
        {
            "code": e.code,
            "name": e.name,
            "description": e.description,
        }
    )
    response.content_type = "application/json"
    print("400 error:", response, file=sys.stderr)
    return response

def main():
    if sys.argv[1] == None:
        print("Port missing\n Correct Usage: python3 udf_server.py <port> [functions_path] [tmp_path]")
    elif sys.argv[2] == None:
        print("Warning: Path to the functions directory is missing\nBy default the path will be the current directory")
        print("Correct Usage: python3 udf_server.py <port> [functions_path] [tmp_path]")
    elif sys.argv[3] == None:
        print(
            "Warning: Path to the temporary directory is missing\nBy default the path will be the current directory"
        )
        print("Correct Usage: python3 udf_server.py <port> [functions_path] [tmp_path]")
    else:
        setup(sys.argv[2], sys.argv[3])
		if DEBUG_MODE:
            print("using host: 0.0.0.0 port:", sys.argv[1])
        app.run(host="0.0.0.0", port=int(sys.argv[1]))

if __name__ == "__main__":
    main()