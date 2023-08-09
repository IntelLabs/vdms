import time
import cv2


def run(settings, message, input_params):
    ipfilename = message
    format = message.strip().split(".")[-1]

    t1 = time.time()

    opfilename = settings["opfile"] + str(t1) + "." + format

    img = cv2.imread(ipfilename)

    img = cv2.flip(img, 0)

    cv2.imwrite(opfilename, img)

    return (time.time() - t1), opfilename
