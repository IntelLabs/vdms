import time
import cv2
import os


def run(settings, message, input_params):
    ipfilename = message
    format = message.strip().split(".")[-1]

    t1 = time.time()

    opfilename = settings["opfile"] + str(t1) + "." + format

    if not os.path.exists(ipfilename):
        raise Exception(f"Flip error: File ipfilename: {ipfilename} does not exist")

    img = cv2.imread(ipfilename)

    img = cv2.flip(img, 0)

    parent_dir = os.path.dirname(opfilename)

    if not os.path.exists(parent_dir):
        raise Exception(
            f"Flip error: Directory for opfilename: {opfilename} does not exist"
        )

    cv2.imwrite(opfilename, img)

    return opfilename
