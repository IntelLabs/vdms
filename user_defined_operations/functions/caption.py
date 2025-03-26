import cv2
import skvideo.io
import time


def run(settings, message, input_params):
    ipfilename = message
    format = message.strip().split(".")[-1]

    t1 = time.time()
    opfilename = settings["opfile"] + str(t1) + "." + format
    vs = cv2.VideoCapture(ipfilename)

    video = skvideo.io.FFmpegWriter(opfilename, {"-pix_fmt": "bgr24"})

    while True:
        (grabbed, frame) = vs.read()
        if not grabbed:
            print("[INFO] no frame read from stream - exiting")
            video.close()
            break

        label = input_params["text"]
        cv2.putText(
            frame, label, (10, 25), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (255, 255, 255), 2
        )

        video.writeFrame(frame)

    return opfilename
