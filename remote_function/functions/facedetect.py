import cv2
import os
import numpy as np

# Get the real directory where this Python file is
currentDir = os.path.realpath(os.path.dirname(__file__))

haarcascade_frontalface_default_path = os.path.join(
    currentDir, "../../resources/haarcascade_frontalface_default.xml"
)

if not os.path.exists(haarcascade_frontalface_default_path):
    raise Exception(
        f"{haarcascade_frontalface_default_path}: path is invalid in facedetect for the remote function"
    )

face_cascade = cv2.CascadeClassifier(
    # This file is available from OpenCV 'data' directory at
    # https://github.com/opencv/opencv/blob/4.x/data/haarcascades/haarcascade_frontalface_default.xml
    haarcascade_frontalface_default_path
)


def run(entity, options):
    global face_cascade

    image_array = np.frombuffer(entity, dtype=np.uint8)

    img = cv2.imdecode(image_array, cv2.IMREAD_COLOR)
    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    faces = face_cascade.detectMultiScale(gray, 1.1, 4)

    for x, y, w, h in faces:
        cv2.rectangle(img, (x, y), (x + w, y + h), (255, 0, 0), 2)

    success, encoded_img = cv2.imencode(".jpg", img)
    if not success:
        raise ValueError("Failed to encode image.")
    ebytes = encoded_img.tobytes()

    rdict = {"metadata": "None"}

    return ebytes, rdict
