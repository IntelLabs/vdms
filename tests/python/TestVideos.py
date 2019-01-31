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

from threading import Thread
import sys
import os
import urllib
import time
import json
import unittest
import numpy as np
import vdms

hostname = "localhost"
port = 55557

class TestVideos(unittest.TestCase):

    #Methos to insert one image
    def insertVideo(self, db, props=None):

        video_arr = []
        all_queries = []

        fd = open("../test_videos/Megamind.avi", 'rb')
        video_arr.append(fd.read())
        fd.close()

        video_parms = {}

        # adds some prop
        if not props is None:
            props["test_case"] = "test_case_prop"
            video_parms["properties"] = props

        video_parms["codec"]     = "h264"
        video_parms["container"] = "mp4"

        query = {}
        query["AddVideo"] = video_parms

        all_queries.append(query)

        response, res_arr = db.query(all_queries, [video_arr])

        self.assertEqual(len(response), 1)
        self.assertEqual(response[0]["AddVideo"]["status"], 0)

    def test_addVideo(self):
        db = vdms.vdms()
        db.connect(hostname, port)

        all_queries = []
        video_arr = []

        number_of_inserts = 2

        for i in range(0,number_of_inserts):
            #Read Brain Image
            fd = open("../test_videos/Megamind.avi", 'rb')
            video_arr.append(fd.read())
            fd.close()

            op_params_resize = {}
            op_params_resize["height"] = 512
            op_params_resize["width"]  = 512
            op_params_resize["type"] = "resize"

            props = {}
            props["name"] = "video_" + str(i)
            props["doctor"] = "Dr. Strange Love"

            video_parms = {}
            video_parms["properties"] = props
            video_parms["codec"] = "h264"

            query = {}
            query["AddVideo"] = video_parms

            all_queries.append(query)

        response, obj_array = db.query(all_queries, [video_arr])
        self.assertEqual(len(response), number_of_inserts)
        for i in range(0, number_of_inserts):
            self.assertEqual(response[i]["AddVideo"]["status"], 0)

    def test_findVideo(self):
        db = vdms.vdms()
        db.connect(hostname, port)

        prefix_name = "video_1_"

        number_of_inserts = 2

        for i in range(0,number_of_inserts):
            props = {}
            props["name"] = prefix_name + str(i)
            self.insertVideo(db, props=props)

        all_queries = []

        for i in range(0,number_of_inserts):
            constraints = {}
            constraints["name"] = ["==", prefix_name + str(i)]

            video_parms = {}
            video_parms["constraints"] = constraints

            query = {}
            query["FindVideo"] = video_parms

            all_queries.append(query)

        response, vid_array = db.query(all_queries)

        self.assertEqual(len(response), number_of_inserts)
        self.assertEqual(len(vid_array), number_of_inserts)
        for i in range(0, number_of_inserts):
            self.assertEqual(response[i]["FindVideo"]["status"], 0)

    def test_findVideoResults(self):
        db = vdms.vdms()
        db.connect(hostname, port)

        prefix_name = "resvideo_1_"

        number_of_inserts = 2

        for i in range(0,number_of_inserts):
            props = {}
            props["name"] = prefix_name + str(i)
            self.insertVideo(db, props=props)

        all_queries = []

        for i in range(0,number_of_inserts):
            constraints = {}
            constraints["name"] = ["==", prefix_name + str(i)]

            results = {}
            results["list"] = ["name"]

            video_parms = {}
            video_parms["constraints"] = constraints
            video_parms["results"] = results

            query = {}
            query["FindVideo"] = video_parms

            all_queries.append(query)

        response, vid_array = db.query(all_queries)

        self.assertEqual(len(response), number_of_inserts)
        self.assertEqual(len(vid_array), number_of_inserts)
        for i in range(0, number_of_inserts):
            self.assertEqual(response[i]["FindVideo"]["status"], 0)

    def test_addVideoWithLink(self):
        db = vdms.vdms()
        db.connect(hostname, port)

        all_queries = []

        props = {}
        props["name"] = "Luis"
        props["lastname"] = "Ferro"
        props["age"] = 27

        addEntity = {}
        addEntity["_ref"] = 32
        addEntity["properties"] = props
        addEntity["class"] = "AwPeopleVid"

        query = {}
        query["AddEntity"] = addEntity

        all_queries.append(query)

        props = {}
        props["name"] = "Luis"
        props["lastname"] = "Malo"
        props["age"] = 27

        link = {}
        link["ref"] = 32
        link["direction"] = "in"
        link["class"] = "Friends"

        addVideo = {}
        addVideo["properties"] = props
        addVideo["link"] = link

        imgs_arr = []

        fd = open("../test_videos/Megamind.avi", 'rb')
        imgs_arr.append(fd.read())
        fd.close()

        img_params = {}

        query = {}
        query["AddVideo"] = addVideo

        all_queries.append(query)

        response, res_arr = db.query(all_queries, [imgs_arr])

        self.assertEqual(response[0]["AddEntity"]["status"], 0)
        self.assertEqual(response[1]["AddVideo"]["status"], 0)

    def test_findVid_multiple_results(self):
        db = vdms.vdms()
        db.connect(hostname, port)

        prefix_name = "vid_multiple"

        number_of_inserts = 4
        for i in range(0,number_of_inserts):
            props = {}
            props["name"] = prefix_name
            self.insertVideo(db, props=props)

        constraints = {}
        constraints["name"] = ["==", prefix_name]

        results = {}
        results["list"] = ["name"]

        img_params = {}
        img_params["constraints"] = constraints

        query = {}
        query["FindVideo"] = img_params

        all_queries = []
        all_queries.append(query)

        response, vid_arr = db.query(all_queries)

        self.assertEqual(len(vid_arr), number_of_inserts)
        self.assertEqual(response[0]["FindVideo"]["status"], 0)
        self.assertEqual(response[0]["FindVideo"]["returned"], number_of_inserts)

    def test_findVideoNoBlob(self):
        db = vdms.vdms()
        db.connect(hostname, port)

        prefix_name = "fvid_no_blob_"

        for i in range(0,2):
            props = {}
            props["name"] = prefix_name + str(i)
            self.insertVideo(db, props=props)

        all_queries = []

        for i in range(0,2):
            constraints = {}
            constraints["name"] = ["==", prefix_name + str(i)]

            results = {}
            results["blob"] = False
            results["list"] = ["name"]

            img_params = {}
            img_params["constraints"] = constraints
            img_params["results"] = results

            query = {}
            query["FindVideo"] = img_params

            all_queries.append(query)

        response, img_array = db.query(all_queries)

        self.assertEqual(response[0]["FindVideo"]["status"], 0)
        self.assertEqual(response[1]["FindVideo"]["status"], 0)
        self.assertEqual(len(img_array), 0)

    def test_updateVideo(self):
        db = vdms.vdms()
        db.connect(hostname, port)

        prefix_name = "fvid_update_"

        for i in range(0,2):
            props = {}
            props["name"] = prefix_name + str(i)
            self.insertVideo(db, props=props)

        all_queries = []

        constraints = {}
        constraints["name"] = ["==", prefix_name + str(0)]

        props = {}
        props["name"] = "simg_update_0"

        img_params = {}
        img_params["constraints"] = constraints
        img_params["properties"] = props

        query = {}
        query["UpdateVideo"] = img_params

        all_queries.append(query)

        response, img_array = db.query(all_queries)

        self.assertEqual(response[0]["UpdateVideo"]["count"], 1)
        self.assertEqual(len(img_array), 0)
