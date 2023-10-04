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

for entry in os.scandir("functions"):
    if entry.is_file():
        string = f"from functions import {entry.name}"[:-3]
        exec(string)

app = Flask(__name__)

count = 0


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

    tmpfile = "tmpfile" + uuid.uuid1().hex + "." + str(format)

    image_data.save(tmpfile)

    udf = globals()[json_data["id"]]
    r_img = udf.run(tmpfile, format, json_data)

    return_string = cv2.imencode("." + str(format), r_img)[1].tostring()
    os.remove(tmpfile)
    return return_string


@app.route("/video", methods=["POST"])
def video_api():
    json_data = json.loads(request.form["jsonData"])
    video_data = request.files["videoData"]
    format = json_data["format"]

    tmpfile = "tmpfile" + uuid.uuid1().hex + "." + str(format)
    video_data.save(tmpfile)

    udf = globals()[json_data["id"]]
    response_file = udf.run(tmpfile, format, json_data)

    os.remove(tmpfile)

    @after_this_request
    def remove_tempfile(response):
        try:
            os.remove(response_file)
        except Exception as e:
            print("File cannot be deleted or not present")
        return response

    try:
        return send_file(response_file, as_attachment=True, download_name=response_file)
    except Exception as e:
        print(str(e))
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
    print("400 error:", response)
    return response


if __name__ == "__main__":
    if sys.argv[1] == None:
        print("Port missing\n Correct Usage: python3 udf_server.py <port>")
    else:
        app.run(host="0.0.0.0", port=int(sys.argv[1]))
