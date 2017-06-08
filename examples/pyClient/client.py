#! /usr/bin/python
from threading import Thread
import sys
import os
import urllib
import time

import athena # Import athena

def clientThread(thId):

    response = athena.query(
            "{HERE GOES YOUR JSON QUERY from Th " + str(thId) + "}")

    print "Thread " + str(thId) + ": " + response


for i in range(1,100):
    thread = Thread(target=clientThread,args=(i,) )
    thread.start()

