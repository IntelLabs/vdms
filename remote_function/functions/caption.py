import imageio.v3 as iio
import skvideo.io
import cv2
import os
import uuid


def run(entity, options, tmp_dir_path=""):
    fname = os.path.join(tmp_dir_path, "tmpfile" + uuid.uuid1().hex + ".mp4")

    label = options["text"]
    video = skvideo.io.FFmpegWriter(fname)
    for frame in iio.imiter(entity, format_hint=".mp4"):
        cv2.putText(
            frame, label, (10, 25), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (255, 255, 255), 2
        )
        video.writeFrame(frame)

    video.close()

    ebytes = ""
    with open(fname, "rb") as f:
        ebytes = f.read()

    # with open('bytefile.mp4', "wb") as out_file:
    #     out_file.write(ebytes)

    os.remove(fname)

    rdict = {"metadata": "None"}

    return ebytes, rdict
