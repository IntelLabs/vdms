import cv2
import numpy as np
from datetime import datetime
from collections import deque
import skvideo.io
import imutils
import time
import json

face_cascade = cv2.CascadeClassifier(
    # This file is available from OpenCV 'data' directory at
    # https://github.com/opencv/opencv/blob/4.x/data/haarcascades/haarcascade_frontalface_default.xml
    "../../user_defined_operations/functions/files/haarcascade_frontalface_default.xml"
)


def facedetectbbox(frame):
    global face_cascade
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    faces = face_cascade.detectMultiScale(gray, 1.1, 4)
    return faces


def run(settings, message, input_params):
    ipfilename = message
    format = message.strip().split(".")[-1]

    if input_params["media_type"] == "video":

        vs = cv2.VideoCapture(ipfilename)
        frameNum = 1
        metadata = {}
        while True:
            (grabbed, frame) = vs.read()
            if not grabbed:
                print("[INFO] no frame read from stream - exiting")
                break

            if input_params["otype"] == "face":
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
        r = json.dumps(response)
        print(response)
        print(r)
        return r

    else:
        tdict = {}
        img = cv2.imread(ipfilename)
        if input_params["otype"] == "face":
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
        print(response)
        print(r)

        return r
