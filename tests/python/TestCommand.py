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

import time
import unittest
import vdms
import os

class TestCommand(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        super(TestCommand, self).__init__(*args, **kwargs)
        # Flag for displaying debug messages
        self.verbose = False

        # VDMS Server Info
        self.hostname = "localhost"
        self.port = 55565
        aws_port = 55564

        db_up = False
        attempts = 0
        db = vdms.vdms()
        while not db_up:
            try:
                db.connect(self.hostname, self.port)
                db.disconnect()
                db_up = True
                if attempts > 0:
                    print("Connection to VDMS successful.")
            except Exception as e:
                if e.strerror == "Connection refused":
                    try:
                        db = vdms.vdms()
                        db.connect(self.hostname, aws_port)
                        db.disconnect()
                        db_up = True
                        if attempts > 0:
                            print("Connection to VDMS successful.")
                        self.port = aws_port
                    except Exception as e:
                        print(
                            "Attempt number",
                            attempts,
                            "to connect to VDMS failed, retrying...",
                        )
                        attempts += 1
                        time.sleep(1)  # sleeps 1 second
                else:
                    print(
                        "Attempt number",
                        attempts,
                        "to connect to VDMS failed, retrying...",
                    )
                    attempts += 1
                    time.sleep(1)  # sleeps 1 second

            if attempts > 10:
                print("Failed to connect to VDMS after 10 attempts")
                exit()

    def create_connection(self):
        db = vdms.vdms()

        if db.is_connected() is False:
            db.connect(self.hostname, self.port)
            if self.verbose is True:
                print(
                    "Connection created for hostname:",
                    self.hostname,
                    "and port:",
                    str(self.port),
                )
        else:
            if self.verbose is True:
                print(
                    "Connection is already active for hostname:",
                    self.hostname,
                    "and port:",
                    str(self.port),
                )
        return db

    def disconnect(self, db):
        if db is not None:
            if db.is_connected() is True:
                db.disconnect()
                if self.verbose is True:
                    print(
                        "Disconnection done for hostname:",
                        self.hostname,
                        "and port:",
                        str(self.port),
                    )
            else:
                if self.verbose is True:
                    print(
                        "disconnect() was not executed for hostname:",
                        self.hostname,
                        "and port:",
                        str(self.port),
                    )

    def addEntity(
        self,
        class_name,
        db,
        properties=None,
        constraints=None,
        blob=False,  # Generic blob
        check_status=True,
    ):
        all_queries = []
        all_blobs = []

        query = self.create_entity(
            "AddEntity",
            class_str=class_name,
            props=properties,
            constraints=constraints,
            blob=blob,
        )
        all_queries.append(query)

        if blob:
            blob_arr = []
            fd = open(os.path.join(self.find_tests_dir(),"test_images/brain.png"), "rb")
            blob_arr.append(fd.read())
            fd.close()
            all_blobs.append(blob_arr)

        response, res_arr = db.query(all_queries, all_blobs)

        if check_status:
            self.assertEqual(response[0]["AddEntity"]["status"], 0)

        return response, res_arr

    def create_descriptor_set(self, name, dim, metric="L2", engine="FaissFlat"):
        all_queries = []

        descriptor_set = {}
        descriptor_set["name"] = name
        descriptor_set["dimensions"] = dim
        descriptor_set["metric"] = metric
        descriptor_set["engine"] = engine

        query = {}
        query["AddDescriptorSet"] = descriptor_set

        all_queries.append(query)
        return all_queries

    def create_entity(
        self,
        command_str,
        ref=None,
        class_str=None,
        props=None,
        blob=False,
        constraints=None,
        unique=False,
        results=None,
        link=None,
    ):
        entity = {}
        if unique:
            entity["unique"] = unique

        if results is not None:
            entity["results"] = results

        if link is not None:
            entity["link"] = link

        if ref is not None:
            entity["_ref"] = ref

        if props not in [None, {}]:
            entity["properties"] = props

        if class_str is not None:
            entity["class"] = class_str

        if constraints is not None:
            entity["constraints"] = constraints

        if blob and command_str == "AddEntity":
            entity["blob"] = blob

        query = {command_str: entity}
        return query

    # Check the signature of any PNG file
    # by going through the first eight bytes of data
    #    (decimal)              137  80  78  71  13  10  26  10
    #    (hexadecimal)           89  50  4e  47  0d  0a  1a  0a
    #    (ASCII C notation)    \211   P   N   G  \r  \n \032 \n
    def verify_png_signature(self, img):
        self.assertFalse(len(img) < 8)
        self.assertEqual(img[0], 137)
        self.assertEqual(img[1], 80)
        self.assertEqual(img[2], 78)
        self.assertEqual(img[3], 71)
        self.assertEqual(img[4], 13)
        self.assertEqual(img[5], 10)
        self.assertEqual(img[6], 26)
        self.assertEqual(img[7], 10)

    def shouldSkipRemotePythonTest():
        return unittest.skipIf(
            os.environ.get("VDMS_SKIP_REMOTE_PYTHON_TESTS") is not None
            and os.environ.get("VDMS_SKIP_REMOTE_PYTHON_TESTS").upper() == "TRUE",
            "VDMS_SKIP_REMOTE_PYTHON_TESTS env var is set to True",
        )
    

    def find_tests_dir(self) -> str:
        tests_dir_path = ""

        # Get the path to the tests directory
        dir_path = os.getcwd()
        max_levels = 2 # To prevent the access to another directories
        counter = 0
        while os.path.basename(dir_path) != "tests" and counter < max_levels:
            dir_path = os.path.dirname(dir_path)
            counter = counter+1
        if os.path.basename(dir_path) == "tests":
            tests_dir_path = dir_path
        else:
            raise Exception("Error: tests directory was not found")

        return tests_dir_path


