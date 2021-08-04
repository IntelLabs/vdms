#
# The MIT License
#
# @copyright Copyright (c) 2017 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction,
# including without limitation the rights to use, copy, modify,
# merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
# ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

import sys
import os
import urllib
import time
import json
import unittest
import vdms

class TestCommand(unittest.TestCase):

    def __init__(self, *args, **kwargs):
        super(TestCommand, self).__init__(*args, **kwargs)

        # VDMS Server Info
        self.hostname = "localhost"
        self.port = 55557

        db_up = False
        attempts = 0
        while(not db_up):
            try:
                db = vdms.vdms()
                db.connect(self.hostname, self.port)
                db.disconnect()
                db_up = True
                if (attempts > 0):
                    print("Connection to VDMS successful.")
            except:
                print("Attempt", attempts,
                      "to connect to VDMS failed, retying...")
                attempts += 1
                time.sleep(1) # sleeps 1 second

            if attempts > 10:
                print("Failed to connect to VDMS after 10 attempts")
                exit()

    def create_connection(self):

        db = vdms.vdms()
        db.connect(self.hostname, self.port)

        return db

    def addEntity(self, class_name, properties=None,
                  constraints=None,
                  blob = False, # Generic blob
                  check_status=True):

        addEntity = {}
        addEntity["class"] = class_name

        if properties != None:
            addEntity["properties"] = properties
        if constraints != None:
            addEntity["constraints"] = constraints

        query = {}
        query["AddEntity"] = addEntity

        all_queries = []
        all_queries.append(query)

        db = self.create_connection()

        if not blob:
            response, res_arr = db.query(all_queries)
        else:
            blob_arr = []
            fd = open("../test_images/brain.png", 'rb')
            blob_arr.append(fd.read())
            fd.close()

            addEntity["blob"] = True

            response, res_arr = db.query(all_queries, [blob_arr])

        if check_status:
            self.assertEqual(response[0]["AddEntity"]["status"], 0)

        return response, res_arr
