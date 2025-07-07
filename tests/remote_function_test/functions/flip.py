import cv2
import numpy as np


def run(entity, options):
    image_array = np.frombuffer(entity, dtype=np.uint8)

    img = cv2.imdecode(image_array, cv2.IMREAD_COLOR)

    img = cv2.flip(img, 0)

    success, encoded_img = cv2.imencode(".jpg", img)
    if not success:
        raise ValueError("Failed to encode image.")
    ebytes = encoded_img.tobytes()

    rdict = {"metadata": "None"}

    return ebytes, rdict
