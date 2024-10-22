import cv2
import skvideo.io
import time
import sys

DEBUG_MODE=True

def run(settings, message, input_params, tmp_dir_path, functions_path):
    if DEBUG_MODE:
        print("Temporary path:", tmp_dir_path, file=sys.stderr)
        print("Functions path:", functions_path, file=sys.stderr)
        print("Settings:", settings, file=sys.stderr)
        print("message:", message, file=sys.stderr)
        print("input_params", input_params, file=sys.stderr)
    ipfilename = message
    format = message.strip().split(".")[-1]

    t1 = time.time()
    opfilename = settings["opfile"] + str(t1) + "." + format
    if DEBUG_MODE:
        print(opfilename, file=sys.stderr)
    vs = cv2.VideoCapture(ipfilename)

    video = skvideo.io.FFmpegWriter(opfilename, {"-pix_fmt": "bgr24"})
    i = 0
    while True:
        (grabbed, frame) = vs.read()
        if not grabbed:
            print("[INFO] no frame read from stream - exiting")
            video.close()
            # sys.exit(0)
            break

        label = input_params["text"]
        cv2.putText(
            frame, label, (10, 25), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (255, 255, 255), 2
        )

        video.writeFrame(frame)

    return opfilename, None
