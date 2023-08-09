import time
import cv2


def run(ipfilename, format, options):
    img = cv2.imread(ipfilename)

    img = cv2.flip(img, 0)

    return img
