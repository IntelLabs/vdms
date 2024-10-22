import cv2
import uuid
import json
import os
import sys

DEBUG_MODE = True

face_cascade = None


def set_face_cascade(functions_path):
    global face_cascade
    haarcascade_frontalface_default_path = os.path.join(
        functions_path, "files/haarcascade_frontalface_default.xml"
    )

    if not os.path.exists(haarcascade_frontalface_default_path):
        raise Exception(f"{haarcascade_frontalface_default_path}: path is invalid")

    face_cascade = cv2.CascadeClassifier(
        # This file is available from OpenCV 'data' directory at
        # https://github.com/opencv/opencv/blob/4.x/data/haarcascades/haarcascade_frontalface_default.xml
        haarcascade_frontalface_default_path
    )


def facedetectbbox(frame):
    global face_cascade
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    faces = face_cascade.detectMultiScale(gray, 1.1, 4)
    return faces


def run(ipfilename, format, options, tmp_dir_path, functions_path):
    if DEBUG_MODE:
        print("Using old metadata **** Temporary path:", tmp_dir_path, file=sys.stderr)
        print("Functions path:", functions_path, file=sys.stderr)
        print("options:", options, file=sys.stderr)
        print("format:", format, file=sys.stderr)
        print("ipfilename:", ipfilename, file=sys.stderr)

    set_face_cascade(functions_path)

    if options["media_type"] == "video":

        vs = cv2.VideoCapture(ipfilename)
        frameNum = 1
        metadata = {}
        while True:
            (grabbed, frame) = vs.read()
            if not grabbed:
                print("[INFO] no frame read from stream - exiting")
                break

            if options["otype"] == "face":
                faces = facedetectbbox(frame)
                if len(faces) > 0:
                    face = faces[0]
                    tdict = {
                        "x": int(face[0]),
                        "y": int(face[1]),
                        "height": int(face[2]),
                        "width": int(face[3]),
                        "object": "face",
                        "object_det": {"emotion": "happy", "age": 30},
                    }

                    metadata[frameNum] = {"frameId": frameNum, "bbox": tdict}
                    frameNum += 1

                    if frameNum == 3:
                        break
            else:
                faces = facedetectbbox(frame)
                if len(faces) > 0:
                    face = faces[0]
                    tdict = {
                        "x": int(face[0]) + 3,
                        "y": int(face[1]) + 5,
                        "height": int(face[2]) + 10,
                        "width": int(face[3]) + 30,
                        "object": "car",
                        "object_det": {"color": "red"},
                    }

                    metadata[frameNum] = {"frameId": frameNum, "bbox": tdict}
                    frameNum += 1

                    if frameNum == 3:
                        break

        response = {"opFile": ipfilename, "metadata": metadata}

        jsonfile = os.path.join(tmp_dir_path, "jsonfile" + uuid.uuid1().hex + ".json")
        with open(jsonfile, "w") as f:
            json.dump(response, f, indent=4)
        return ipfilename, jsonfile

    else:
        tdict = {}
        img = cv2.imread(ipfilename)
        if options["otype"] == "face":
            faces = facedetectbbox(img)
            if len(faces) > 0:
                face = faces[0]
                tdict = {
                    "x": int(face[0]),
                    "y": int(face[1]),
                    "height": int(face[2]),
                    "width": int(face[3]),
                    "object": "face",
                    "object_det": {"emotion": "happy", "age": 30},
                }
        else:
            faces = facedetectbbox(img)
            if len(faces) > 0:
                face = faces[0]
                tdict = {
                    "x": int(face[0]) + 3,
                    "y": int(face[1]) + 5,
                    "height": int(face[2]) + 10,
                    "width": int(face[3]) + 30,
                    "object": "car",
                    "object_det": {"color": "red"},
                }

        response = {"opFile": ipfilename, "metadata": tdict}

        r = json.dumps(response)
        return img, r
