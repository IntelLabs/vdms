import time
import cv2
import os

haarcascade_frontalface_default_path = (
    "../../resources/haarcascade_frontalface_default.xml"
)

if not os.path.exists(haarcascade_frontalface_default_path):
    raise Exception(
        f"{haarcascade_frontalface_default_path}: path is invalid in facedetect for the user defined operations"
    )

face_cascade = cv2.CascadeClassifier(
    # This file is available from OpenCV 'data' directory at
    # https://github.com/opencv/opencv/blob/4.x/data/haarcascades/haarcascade_frontalface_default.xml
    haarcascade_frontalface_default_path
)


def run(settings, message, input_params, tmp_dir_path):
    global face_cascade

    ipfilename = message
    format = message.strip().split(".")[-1]
    t1 = time.time()

    opfilename = settings["opfile"] + str(t1) + "." + format

    if not os.path.exists(ipfilename):
        raise Exception(
            f"Facedetect error: File ipfilename: {ipfilename} does not exist"
        )

    img = cv2.imread(ipfilename)

    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)

    faces = face_cascade.detectMultiScale(gray, 1.1, 4)

    for x, y, w, h in faces:
        cv2.rectangle(img, (x, y), (x + w, y + h), (255, 0, 0), 2)

    parent_dir = os.path.dirname(opfilename)
    if not os.path.exists(parent_dir):
        raise Exception(
            f"Facedetect error: Directory for opfilename: {opfilename} does not exist"
        )

    cv2.imwrite(opfilename, img)

    return opfilename, None
