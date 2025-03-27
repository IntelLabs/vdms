import cv2
import time


def run(settings, message, input_params):
    ipfilename = message
    format = message.strip().split(".")[-1]

    t1 = time.time()
    opfilename = settings["opfile"] + str(t1) + "." + format
    vs = cv2.VideoCapture(ipfilename)

    frame_width = int(vs.get(3))
    frame_height = int(vs.get(4))
    video_fps = int(vs.get(cv2.CAP_PROP_FPS))

    video = cv2.VideoWriter(
        opfilename,
        cv2.VideoWriter_fourcc(*"XVID"),
        video_fps,
        (frame_width, frame_height),
    )

    while True:
        (grabbed, frame) = vs.read()
        if not grabbed:
            print("[INFO] no frame read from stream - exiting")
            break

        label = input_params["text"]
        cv2.putText(
            frame, label, (10, 25), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (255, 255, 255), 2
        )

        video.write(frame)
    video.release()

    return opfilename
