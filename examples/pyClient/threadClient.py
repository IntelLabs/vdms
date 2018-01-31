#! /usr/bin/python
from threading import Thread
import sys
import os
import urllib
import time

import vdms # Import vdms

def clientThread(thId):

    response = vdms.query(
            "{HERE GOES YOUR JSON QUERY from Th " + str(thId) + "}")

    print "Thread " + str(thId) + ": " + response

for i in range(1,1000):
    thread = Thread(target=clientThread,args=(i,) )
    thread.start()

