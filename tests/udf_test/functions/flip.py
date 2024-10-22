import time
import cv2
import os
import sys

DEBUG_MODE = True


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
        print("Flip: ipfilename", ipfilename)
    if not os.path.exists(ipfilename):
        raise Exception(f"Flip error: File ipfilename: {ipfilename} does not exist")

    img = cv2.imread(ipfilename)

    img = cv2.flip(img, 0)

    parent_dir = os.path.dirname(opfilename)
    if DEBUG_MODE:
        print("Flip: parent_dir", parent_dir)
    if not os.path.exists(parent_dir):
        raise Exception(
            f"Flip error: Directory for opfilename: {opfilename} does not exist"
        )

    cv2.imwrite(opfilename, img)

    return opfilename, None
