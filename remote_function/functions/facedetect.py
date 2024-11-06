import cv2
import os
import sys

DEBUG_MODE = True


def run(ipfilename, format, options, tmp_dir_path, functions_path):
    if DEBUG_MODE:
        print("Temporary path:", tmp_dir_path, file=sys.stderr)
        print("Functions path:", functions_path, file=sys.stderr)
        print("options:", options, file=sys.stderr)
        print("format:", format, file=sys.stderr)
        print("ipfilename", ipfilename, file=sys.stderr)

    haarcascade_frontalface_default_path = os.path.join(
        functions_path, "files/haarcascade_frontalface_default.xml"
    )

    if not os.path.exists(haarcascade_frontalface_default_path):
        raise Exception(f"{haarcascade_frontalface_default_path}: path is invalid")

    face_cascade = cv2.CascadeClassifier(
        # This file is available from OpenCV 'data' directory at
        # https://github.com/opencv/opencv/blob/4.x/data/haarcascades/haarcascade_frontalface_default.xml
        # "functions/files/haarcascade_frontalface_default.xml"
        haarcascade_frontalface_default_path
    )

    if DEBUG_MODE:
        print("Facedetect: ipfilename", ipfilename)
    if not os.path.exists(ipfilename):
        raise Exception(
            f"Facedetect error: File ipfilename: {ipfilename} does not exist"
        )

    img = cv2.imread(ipfilename)
    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    faces = face_cascade.detectMultiScale(gray, 1.1, 4)

    for x, y, w, h in faces:
        cv2.rectangle(img, (x, y), (x + w, y + h), (255, 0, 0), 2)

    return img, None
