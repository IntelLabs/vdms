#! /usr/bin/python
from threading import Thread
import sys
import os
import urllib
import time
import json
import unittest

import vdms


if len(sys.argv) != 2:
    print "You must provide a json file"
else:
    hostname = "localhost"

    db = vdms.VDMS()
    db.connect(hostname)

    with open(sys.argv[1]) as json_file:
      query = json.load(json_file)

    response, img_array = db.query(query)
    print vdms.aux_print_json(response)