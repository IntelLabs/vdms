import cv2
import uuid
import os
import sys

DEBUG_MODE = False


def run(ipfilename, format, options, tmp_dir_path, functions_path):
    if DEBUG_MODE:
        print("Temporary path:", tmp_dir_path, file=sys.stderr)
        print("Functions path:", functions_path, file=sys.stderr)
        print("options:", options, file=sys.stderr)
        print("format:", format, file=sys.stderr)
        print("ipfilename", ipfilename, file=sys.stderr)
    opfilename = os.path.join(
        tmp_dir_path, "tmpfile" + uuid.uuid1().hex + "." + str(format)
    )
    if DEBUG_MODE:
        print("opfilename:", opfilename, file=sys.stderr)
    vs = cv2.VideoCapture(ipfilename)
    frame_width = int(vs.get(3))
    frame_height = int(vs.get(4))

    video = cv2.VideoWriter(
        opfilename, cv2.VideoWriter_fourcc(*"XVID"), 30, (frame_width, frame_height)
    )

    if DEBUG_MODE:
        print(options, file=sys.stderr)

    while True:
        (grabbed, frame) = vs.read()
        if not grabbed:
            print("[INFO] no frame read from stream - exiting")
            break

        label = options["text"]
        cv2.putText(
            frame, label, (10, 25), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (255, 255, 255), 2
        )

        video.write(frame)
    video.release()

    return opfilename, None
