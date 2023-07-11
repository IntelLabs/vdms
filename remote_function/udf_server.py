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

    tmpfile = "tmpfile" + str(datetime.now()) + "." + str(format)

    image_data.save(tmpfile)

    udf = globals()[json_data["id"]]
    r_img = udf.run(tmpfile, format, json_data)

    return_string = cv2.imencode("." + str(format), r_img)[1].tostring()
    os.remove(tmpfile)
    return return_string


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
