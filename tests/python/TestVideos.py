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

class TestAddVideo(unittest.TestCase):

    #Methos to insert one image
    def insertVideo(self, db, props=None, collections=None, format="png"):

        video_arry = []
        all_queries = []

        fd = open("../test_videos/Megamind.avi")
        video_arry.append(fd.read())

        video_parms = {}

        # adds some prop
        if not props is None:
            props["test_case"] = "test_case_prop"
            video_parms["properties"] = props

        if not collections is None:
            video_parms["collections"] = collections

        video_parms["format"] = format

        query = {}
        query["AddVideo"] = video_parms

        all_queries.append(query)

        response, res_arr = db.query(all_queries, [video_arry])

        # Check success
        response = json.loads(response)
        self.assertEqual(response[0]["AddVideo"]["status"], 0)

    def test_AddVideo(self):
        db = vdms.VDMS()
        db.connect(hostname, port)

        all_queries = []
        video_arry = []

        number_of_inserts = 2

        for i in range(0,number_of_inserts):
            #Read Brain Image
            fd = open("../test_videos/Megamind.avi")
            video_arry.append(fd.read())

            op_params_resize = {}
            op_params_resize["height"] = 512
            op_params_resize["width"]  = 512
            op_params_resize["type"] = "resize"

            props = {}
            props["name"] = "brain_" + str(i)
            props["doctor"] = "Dr. Strange Love"

            video_parms = {}
            video_parms["properties"] = props
            video_parms["operations"] = [op_params_resize]
            video_parms["format"] = "png"

            query = {}
            query["AddVideo"] = video_parms

            all_queries.append(query)

        response, vid_array = db.query(all_queries, [video_arry])

        response = json.loads(response)
        self.assertEqual(len(response), number_of_inserts)
        for i in range(0, number_of_inserts):
            self.assertEqual(response[i]["AddVideo"]["status"], 0)

    def test_findEntityImage(self):
        db = vdms.VDMS()
        db.connect(hostname, port)

        prefix_name = "fent_brain_"

        for i in range(0,2):
            props = {}
            props["name"] = prefix_name + str(i)
            self.insertVideo(db, props=props)

        all_queries = []

        for i in range(0,2):
            constraints = {}
            constraints["name"] = ["==", prefix_name + str(i)]

            results = {}
            results["list"] = ["name"]

            video_parms = {}
            video_parms["constraints"] = constraints
            video_parms["results"] = results
            video_parms["class"] = "VD:IMG"

            query = {}
            query["FindEntity"] = video_parms

            all_queries.append(query)

        response, vid_array = db.query(all_queries)
        # print vdms.aux_print_json(response)

        response = json.loads(response)
        self.assertEqual(response[0]["FindEntity"]["status"], 0)
        self.assertEqual(response[1]["FindEntity"]["status"], 0)
        self.assertEqual(response[0]["FindEntity"]["entities"][0]["name"], prefix_name + "0")
        self.assertEqual(response[1]["FindEntity"]["entities"][0]["name"], prefix_name + "1")

    def test_FindVideo(self):
        db = vdms.VDMS()
        db.connect(hostname, port)

        prefix_name = "fimg_brain_"

        for i in range(0,2):
            props = {}
            props["name"] = prefix_name + str(i)
            self.insertVideo(db, props=props)

        all_queries = []

        for i in range(0,2):
            constraints = {}
            constraints["name"] = ["==", prefix_name + str(i)]

            video_parms = {}
            video_parms["constraints"] = constraints


            query = {}
            query["FindVideo"] = video_parms

            all_queries.append(query)

        response, vid_array = db.query(all_queries)
        # print vdms.aux_print_json(response)

        response = json.loads(response)
        self.assertEqual(response[0]["FindVideo"]["status"], 0)
        self.assertEqual(response[1]["FindVideo"]["status"], 0)
        self.assertEqual(len(vid_array
), 2)

    def test_FindVideoResults(self):
        db = vdms.VDMS()
        db.connect(hostname, port)

        prefix_name = "fimg_results_"

        for i in range(0,2):
            props = {}
            props["name"] = prefix_name + str(i)
            self.insertVideo(db, props=props)

        all_queries = []

        for i in range(0,2):
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
        # print vdms.aux_print_json(str(response))

        response = json.loads(response)
        self.assertEqual(response[0]["FindVideo"]["status"], 0)
        self.assertEqual(response[1]["FindVideo"]["status"], 0)
        self.assertEqual(response[0]["FindVideo"]["entities"][0]["name"], prefix_name + "0")
        self.assertEqual(response[1]["FindVideo"]["entities"][0]["name"], prefix_name + "1")
        self.assertEqual(len(vid_array
), 2)

    def test_AddVideoWithLink(self):
        db = vdms.VDMS()
        db.connect(hostname, port)

        all_queries = []

        props = {}
        props["name"] = "Luis"
        props["lastname"] = "Ferro"
        props["age"] = 27

        addEntity = {}
        addEntity["_ref"] = 32
        addEntity["properties"] = props
        addEntity["class"] = "AwesomePeople"

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

        AddVideo = {}
        AddVideo["properties"] = props
        AddVideo["link"] = link

        video_arry = []

        fd = open("../test_videos/Megamind.avi")
        video_arry.append(fd.read())

        video_parms = {}

        query = {}
        query["AddVideo"] = AddVideo

        all_queries.append(query)

        # print json.dumps(all_queries)
        # vdms.aux_print_json(all_queries)

        response, res_arr = db.query(all_queries, [video_arry])
        response = json.loads(response)
        # vdms.aux_print_json(response)

        self.assertEqual(response[0]["AddEntity"]["status"], 0)
        self.assertEqual(response[1]["AddVideo"]["status"], 0)

    def test_FindVideo_multiple_results(self):
        db = vdms.VDMS()
        db.connect(hostname, port)

        prefix_name = "fimg_brain_multiple"

        number_of_inserts = 4
        for i in range(0,number_of_inserts):
            props = {}
            props["name"] = prefix_name
            self.insertVideo(db, props=props)

        constraints = {}
        constraints["name"] = ["==", prefix_name]

        results = {}
        results["list"] = ["name"]

        video_parms = {}
        video_parms["constraints"] = constraints

        query = {}
        query["FindVideo"] = video_parms

        all_queries = []
        all_queries.append(query)

        response, vid_array = db.query(all_queries)
        # print vdms.aux_print_json(response)

        response = json.loads(response)
        self.assertEqual(len(vid_array), number_of_inserts)
        self.assertEqual(response[0]["FindVideo"]["status"], 0)
        self.assertEqual(response[0]["FindVideo"]["returned"], number_of_inserts)

    def test_FindVideoNoBlob(self):
        db = vdms.VDMS()
        db.connect(hostname, port)

        prefix_name = "fvideo_no_blob_"

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

            video_parms = {}
            video_parms["constraints"] = constraints
            video_parms["results"] = results

            query = {}
            query["FindVideo"] = video_parms

            all_queries.append(query)

        response, vid_array = db.query(all_queries)
        # print vdms.aux_print_json(response)

        response = json.loads(response)
        self.assertEqual(response[0]["FindVideo"]["status"], 0)
        self.assertEqual(response[1]["FindVideo"]["status"], 0)
        self.assertEqual(len(vid_array), 0)

    def test_updateVideo(self):
        db = vdms.VDMS()
        db.connect(hostname, port)

        prefix_name = "fvideo_update_"

        for i in range(0,2):
            props = {}
            props["name"] = prefix_name + str(i)
            self.insertVideo(db, props=props)

        all_queries = []

        constraints = {}
        constraints["name"] = ["==", prefix_name + str(0)]

        props = {}
        props["name"] = "simg_update_0"

        video_parms = {}
        video_parms["constraints"] = constraints
        video_parms["properties"] = props

        query = {}
        query["UpdateVideo"] = video_parms

        all_queries.append(query)

        response, vid_array = db.query(all_queries)
        # print vdms.aux_print_json(response)

        response = json.loads(response)
        self.assertEqual(response[0]["UpdateVideo"]["count"], 1)
        self.assertEqual(len(vid_array), 0)

    def ztest_zFindVideoWithCollection(self):
        db = vdms.VDMS()
        db.connect(hostname, port)

        prefix_name = "fvideo_brain_collection_"
        number_of_inserts = 4

        colls = {}
        colls = ["brainScans"]

        for i in range(0,number_of_inserts):
            props = {}
            props["name"] = prefix_name + str(i)

            self.insertVideo(db, props=props, collections=colls)

        all_queries = []

        for i in range(0,1):

            results = {}
            results["list"] = ["name"]

            video_parms = {}
            video_parms["collections"] = ["brainScans"]
            video_parms["results"] = results

            query = {}
            query["FindVideo"] = video_parms

            all_queries.append(query)

        response, vid_array = db.query(all_queries)
        # print vdms.aux_print_json(response)

        response = json.loads(response)
        self.assertEqual(response[0]["FindVideo"]["status"], 0)
        self.assertEqual(len(vid_array), number_of_inserts)
