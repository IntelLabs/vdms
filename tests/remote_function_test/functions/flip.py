import cv2
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

    if DEBUG_MODE:
        print("Flip: ipfilename", ipfilename)
    if not os.path.exists(ipfilename):
        raise Exception(f"Flip error: File ipfilename: {ipfilename} does not exist")

    img = cv2.imread(ipfilename)

    img = cv2.flip(img, 0)

    return img, None
