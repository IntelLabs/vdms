import cv2
import os


def run(ipfilename, format, options, tmp_dir_path):
    if not os.path.exists(ipfilename):
        raise Exception(f"Flip error: File ipfilename: {ipfilename} does not exist")

    img = cv2.imread(ipfilename)

    img = cv2.flip(img, 0)

    return img, None
