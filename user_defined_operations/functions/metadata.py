import cv2
import json
import os

haarcascade_frontalface_default_path = (
    "../resources/haarcascade_frontalface_default.xml"
)

if not os.path.exists(haarcascade_frontalface_default_path):
    raise Exception(
        f"{haarcascade_frontalface_default_path}: path is invalid in metadata for the user defined operations"
    )

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


def run(settings, message, input_params, tmp_dir_path):
    ipfilename = message

    # Extract metadata for video files
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
                    # We use dummy values here as an example to showcase
                    # different values for car.
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

        return r, None
    # Extract metadata for image files
    else:
        tdict = {}
        if not os.path.exists(ipfilename):
            raise Exception(
                f"Metadata error: File ipfilename {ipfilename} does not exist"
            )

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
                # We use dummy values here as an example to showcase
                # different values for car.
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
        return r, None
