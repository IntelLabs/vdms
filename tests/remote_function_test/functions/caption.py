import cv2
import skvideo.io
import uuid
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

    if not os.path.exists(tmp_dir_path):
        raise Exception(f"{tmp_dir_path}: path is invalid")

    opfilename = os.path.join(
        tmp_dir_path, "tmpfile" + uuid.uuid1().hex + "." + str(format)
    )
    print(opfilename)
    vs = cv2.VideoCapture(ipfilename)

    video = skvideo.io.FFmpegWriter(opfilename, {"-pix_fmt": "bgr24"})
    print(options)

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

    return opfilename, None
