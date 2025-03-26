import cv2
import uuid
import os
import skvideo.io


def run(ipfilename, format, options, tmp_dir_path):
    if not os.path.exists(tmp_dir_path):
        raise Exception(f"{tmp_dir_path}: path is invalid")

    opfilename = os.path.join(
        tmp_dir_path, "tmpfile" + uuid.uuid1().hex + "." + str(format)
    )

    vs = cv2.VideoCapture(ipfilename)

    video = skvideo.io.FFmpegWriter(opfilename, {"-pix_fmt": "bgr24"})

    while True:
        (grabbed, frame) = vs.read()
        if not grabbed:
            print("[INFO] no frame read from stream - exiting")
            video.close()
            break

        label = options["text"]
        cv2.putText(
            frame, label, (10, 25), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (255, 255, 255), 2
        )

        video.writeFrame(frame)

    return opfilename, None
