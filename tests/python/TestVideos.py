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

import TestCommand
import unittest


class TestVideos(TestCommand.TestCommand):
    # Method to insert one video
    def insertVideo(self, db, props=None):
        video_arr = []
        all_queries = []

        fd = open("../test_videos/Megamind.avi", "rb")
        video_arr.append(fd.read())
        fd.close()

        video_parms = {}

        # adds some prop
        if not props is None:
            props["test_case"] = "test_case_prop"
            video_parms["properties"] = props

        video_parms["codec"] = "h264"
        video_parms["container"] = "mp4"

        query = {}
        query["AddVideo"] = video_parms

        all_queries.append(query)

        response, res_arr = db.query(all_queries, [video_arr])

        self.assertEqual(len(response), 1)
        self.assertEqual(response[0]["AddVideo"]["status"], 0)

    def test_addVideo(self):
        db = self.create_connection()

        all_queries = []
        video_arr = []

        number_of_inserts = 2

        for i in range(0, number_of_inserts):
            # Read Brain Image
            fd = open("../test_videos/Megamind.avi", "rb")
            video_arr.append(fd.read())
            fd.close()

            op_params_resize = {}
            op_params_resize["height"] = 512
            op_params_resize["width"] = 512
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

    def test_addVideoFromLocalFile_invalid_command(self):
        # The test is meant to fail if both blob and a local file are specified
        db = self.create_connection()

        with open("../test_videos/Megamind.avi", "rb") as fd:
            video_blob = fd.read()

        video_params = {}
        video_params["from_server_file"] = "BigFile.mp4"
        video_params["codec"] = "h264"

        query = {}
        query["AddVideo"] = video_params

        response, obj_array = db.query([query], [[video_blob]])
        self.assertEqual(response[0]["status"], -1)

    def test_addVideoFromLocalFile_file_not_found(self):
        db = self.create_connection()

        video_params = {}
        video_params["from_server_file"] = "BigFile.mp4"
        video_params["codec"] = "h264"

        query = {}
        query["AddVideo"] = video_params

        response, obj_array = db.query([query], [[]])
        self.assertEqual(response[0]["status"], -1)

    @unittest.skip("Skipping class until fixed")
    def test_addVideoFromLocalFile_success(self):
        db = self.create_connection()

        video_params = {}
        video_params["from_server_file"] = "../../tests/videos/Megamind.mp4"
        video_params["codec"] = "h264"

        query = {}
        query["AddVideo"] = video_params

        response, obj_array = db.query([query], [[]])
        self.assertEqual(response[0]["AddVideo"]["status"], 0)

    def test_extractKeyFrames(self):
        db = self.create_connection()

        fd = open("../../tests/videos/Megamind.mp4", "rb")
        video_blob = fd.read()
        fd.close()

        video_name = "video_test_index_frames"

        props = {}
        props["name"] = video_name

        video_params = {}
        video_params["index_frames"] = True
        video_params["properties"] = props
        video_params["codec"] = "h264"

        query = {}
        query["AddVideo"] = video_params

        response, obj_array = db.query([query], [[video_blob]])

        self.assertEqual(response[0]["AddVideo"]["status"], 0)

        entity = {}
        entity["class"] = "VD:KF"
        entity["results"] = {"count": ""}

        query = {}
        query["FindEntity"] = entity

        response, res_arr = db.query([query])

        self.assertEqual(response[0]["FindEntity"]["status"], 0)

        # we know that this video has exactly four key frames
        self.assertEqual(response[0]["FindEntity"]["count"], 4)

    def test_findVideo(self):
        db = self.create_connection()

        prefix_name = "video_1_"

        number_of_inserts = 2

        for i in range(0, number_of_inserts):
            props = {}
            props["name"] = prefix_name + str(i)
            self.insertVideo(db, props=props)

        all_queries = []

        for i in range(0, number_of_inserts):
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

    def test_FindFramesByFrames(self):
        db = self.create_connection()

        prefix_name = "video_2_"

        number_of_inserts = 2

        for i in range(0, number_of_inserts):
            props = {}
            props["name"] = prefix_name + str(i)
            self.insertVideo(db, props=props)

        all_queries = []

        for i in range(0, number_of_inserts):
            constraints = {}
            constraints["name"] = ["==", prefix_name + str(i)]

            video_params = {}
            video_params["constraints"] = constraints
            video_params["frames"] = [f for f in range(0, 10)]

            query = {}
            query["FindFrames"] = video_params

            all_queries.append(query)

        response, img_array = db.query(all_queries)

        self.assertEqual(response[0]["FindFrames"]["status"], 0)
        self.assertEqual(response[1]["FindFrames"]["status"], 0)
        self.assertEqual(len(img_array), 2 * len(video_params["frames"]))

    def test_FindFramesByInterval(self):
        db = self.create_connection()

        prefix_name = "video_3_"

        number_of_inserts = 2

        for i in range(0, number_of_inserts):
            props = {}
            props["name"] = prefix_name + str(i)
            self.insertVideo(db, props=props)

        all_queries = []

        for i in range(0, number_of_inserts):
            constraints = {}
            constraints["name"] = ["==", prefix_name + str(i)]

            number_of_frames = 10
            operations = []
            interval_operation = {}
            interval_operation["type"] = "interval"
            interval_operation["start"] = 0
            interval_operation["stop"] = number_of_frames
            interval_operation["step"] = 1
            operations.append(interval_operation)

            video_params = {}
            video_params["constraints"] = constraints
            video_params["operations"] = operations

            query = {}
            query["FindFrames"] = video_params

            all_queries.append(query)

        response, img_array = db.query(all_queries)

        self.assertEqual(response[0]["FindFrames"]["status"], 0)
        self.assertEqual(response[1]["FindFrames"]["status"], 0)
        self.assertEqual(len(img_array), 2 * number_of_frames)

    def test_FindFramesMissingParameters(self):
        db = self.create_connection()

        constraints = {}
        constraints["name"] = ["==", "video_1"]

        video_params = {}
        video_params["constraints"] = constraints

        query = {}
        query["FindFrames"] = video_params

        all_queries = []
        all_queries.append(query)

        response, img = db.query(all_queries)

        self.assertEqual(response[0]["status"], -1)
        self.assertEqual(img, [])

    def test_FindFramesInvalidParameters(self):
        db = self.create_connection()

        constraints = {}
        constraints["name"] = ["==", "video_1"]

        operations = []
        interval_operation = {}
        interval_operation["type"] = "interval"
        interval_operation["start"] = 10
        interval_operation["stop"] = 20
        interval_operation["step"] = 1
        operations.append(interval_operation)

        video_params = {}
        video_params["constraints"] = constraints
        video_params["operations"] = operations
        video_params["frames"] = [1]

        query = {}
        query["FindFrames"] = video_params

        all_queries = []
        all_queries.append(query)

        response, img = db.query(all_queries)

        self.assertEqual(response[0]["status"], -1)
        self.assertEqual(img, [])

    def test_findVideoResults(self):
        db = self.create_connection()

        prefix_name = "resvideo_1_"

        number_of_inserts = 2

        for i in range(0, number_of_inserts):
            props = {}
            props["name"] = prefix_name + str(i)
            self.insertVideo(db, props=props)

        all_queries = []

        for i in range(0, number_of_inserts):
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
        db = self.create_connection()

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

        fd = open("../test_videos/Megamind.avi", "rb")
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
        db = self.create_connection()

        prefix_name = "vid_multiple"

        number_of_inserts = 4
        for i in range(0, number_of_inserts):
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
        db = self.create_connection()

        prefix_name = "fvid_no_blob_"

        for i in range(0, 2):
            props = {}
            props["name"] = prefix_name + str(i)
            self.insertVideo(db, props=props)

        all_queries = []

        for i in range(0, 2):
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
        db = self.create_connection()

        prefix_name = "fvid_update_"

        for i in range(0, 2):
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
