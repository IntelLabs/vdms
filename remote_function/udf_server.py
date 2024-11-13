from flask import Flask, request, jsonify, send_file, after_this_request
import cv2
import json
from datetime import datetime, timezone
import os
import sys
import uuid
from zipfile import ZipFile
from werkzeug.utils import secure_filename

for entry in os.scandir("functions"):
    if entry.is_file():
        string = f"from functions import {entry.name}"[:-3]
        exec(string)

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

    tmpfile = secure_filename("tmpfile" + uuid.uuid1().hex + "." + str(format))

    image_data.save(tmpfile)

    r_img, r_meta = "", ""

    udf = globals()[json_data["id"]]
    if "ingestion" in json_data:
        r_img, r_meta = udf.run(tmpfile, format, json_data)
    else:
        r_img = udf.run(tmpfile, format, json_data)

    return_string = cv2.imencode("." + str(format), r_img)[1].tostring()

    if r_meta != "":
        return_string += ":metadata:".encode("utf-8")
        return_string += r_meta.encode("utf-8")

    os.remove(tmpfile)
    if return_string == "" or return_string == None:
        return "error"
    return return_string


@app.route("/video", methods=["POST"])
def video_api():
    json_data = json.loads(request.form["jsonData"])
    video_data = request.files["videoData"]
    format = json_data["format"] if "format" in json_data else "mp4"

    tmpfile = secure_filename("tmpfile" + uuid.uuid1().hex + "." + str(format))
    video_data.save(tmpfile)

    video_file, metadata_file = "", ""

    udf = globals()[json_data["id"]]
    if "ingestion" in json_data:
        video_file, metadata_file = udf.run(tmpfile, format, json_data)
    else:
        video_file = udf.run(tmpfile, format, json_data)

    response_file = "tmpfile" + uuid.uuid1().hex + ".zip"

    with ZipFile(response_file, "w") as zip_object:
        zip_object.write(video_file)
        if metadata_file != "":
            zip_object.write(metadata_file)

    os.remove(tmpfile)

    # Delete the temporary files after the response is sent
    @after_this_request
    def remove_tempfile(response):
        try:
            os.remove(response_file)
            os.remove(video_file)
            os.remove(metadata_file)
        except Exception as e:
            print("Some files cannot be deleted or are not present")
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
