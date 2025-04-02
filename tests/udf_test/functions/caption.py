import cv2
import time


def run(settings, message, input_params):
    ipfilename = message
    format = message.strip().split(".")[-1]

    t1 = time.time()
    opfilename = settings["opfile"] + str(t1) + "." + format
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

        label = input_params["text"]
        cv2.putText(
            frame, label, (10, 25), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (255, 255, 255), 2
        )

        video.write(frame)
    vc.release()
    video.release()

    return opfilename
