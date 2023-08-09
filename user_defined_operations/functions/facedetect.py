import time
import cv2

face_cascade = cv2.CascadeClassifier(
    # This file is available from OpenCV 'data' directory at
    # https://github.com/opencv/opencv/blob/4.x/data/haarcascades/haarcascade_frontalface_default.xml
    "functions/files/haarcascade_frontalface_default.xml"
)


def run(settings, message, input_params):
    global face_cascade
    ipfilename = message
    format = message.strip().split(".")[-1]

    print(ipfilename)
    t1 = time.time()

    opfilename = settings["opfile"] + str(t1) + "." + format

    img = cv2.imread(ipfilename)

    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)

    faces = face_cascade.detectMultiScale(gray, 1.1, 4)

    for x, y, w, h in faces:
        cv2.rectangle(img, (x, y), (x + w, y + h), (255, 0, 0), 2)

    cv2.imwrite(opfilename, img)

    return (time.time() - t1), opfilename
