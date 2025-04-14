import cv2
import uuid
import os


def run(ipfilename, format, options, tmp_dir_path):
    if not os.path.exists(tmp_dir_path):
        raise Exception(f"{tmp_dir_path}: path is invalid")

    opfilename = os.path.join(
        tmp_dir_path, "tmpfile" + uuid.uuid1().hex + "." + str(format)
    )

    vc = cv2.VideoCapture(ipfilename)
    frame_width = int(vc.get(cv2.CAP_PROP_FRAME_WIDTH))
    frame_height = int(vc.get(cv2.CAP_PROP_FRAME_HEIGHT))
    video_fps = vc.get(cv2.CAP_PROP_FPS)

    video = cv2.VideoWriter(
        opfilename,
        cv2.VideoWriter_fourcc(*"mp4v"),
        video_fps,
        (frame_width, frame_height),
    )

    while True:
        (grabbed, frame) = vc.read()
        if not grabbed:
            print("[INFO] no frame read from stream - exiting")
            break

        label = options["text"]
        cv2.putText(
            frame, label, (10, 25), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (255, 255, 255), 2
        )

        video.write(frame)
    vc.release()
    video.release()

    return opfilename, None
