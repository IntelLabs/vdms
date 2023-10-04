import cv2
import numpy as np
from datetime import datetime
from collections import deque
import skvideo.io
import imutils
import uuid


def run(ipfilename, format, options):
    opfilename = "tmpfile" + uuid.uuid1().hex + "." + str(format)
    print(opfilename)
    vs = cv2.VideoCapture(ipfilename)

    video = skvideo.io.FFmpegWriter(opfilename, {"-pix_fmt": "bgr24"})
    print(options)
    i = 0
    while True:
        (grabbed, frame) = vs.read()
        if not grabbed:
            print("[INFO] no frame read from stream - exiting")
            video.close()
            # sys.exit(0)
            break

        label = options["text"]
        cv2.putText(
            frame, label, (10, 25), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (255, 255, 255), 2
        )

        video.writeFrame(frame)

    return opfilename
