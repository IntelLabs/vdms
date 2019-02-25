import cv2
import vdms
import sys
import os
import urllib
import time
import json
import unittest
import numpy as np
import csv
import face_recognition
import util

def get_descriptors(imagePath):

    image = cv2.imread(imagePath)
    image = cv2.resize(image, (0,0), fx=0.6,fy=0.6)
    rgb   = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)

    boxes = face_recognition.face_locations(rgb, model="hog")

    # compute the facial embedding for the face
    encodings = face_recognition.face_encodings(rgb, boxes)

    # Draw a rectangle around the faces
    counter = 0
    for (top, right, bottom, left) in boxes:
        cv2.rectangle(image, (left, top), (right, bottom), (0, 255, 0), 2)
        y = top - 15 if top - 15 > 15 else top + 15
        cv2.putText(image, str(counter), (left, y), cv2.FONT_HERSHEY_SIMPLEX, 0.75, (0, 255, 0), 2)
        counter += 1

    cv2.imwrite("save.jpg", image)
    from IPython.display import Image, display
    display(Image("save.jpg"))


    blob_array = []

    for ele in encodings:
        blob_array.append(np.array(ele).astype('float32').tobytes())

    return blob_array