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

import shutil
import TestCommand
import os


class TestVideos(TestCommand.TestCommand):
    # Check the signature of the mp4 file:
    # ftypisom MP4 ISO Base Media file (MPEG-4)
    # First bytes of the file
    # (decimal)                     102 116 121 112 105 115 111 109
    # (hexadecimal) 4 byte offset +  66  74  79  70  69  73  6F  6D
    def verify_mp4_signature(self, vid):
        self.assertFalse(len(vid) < 12)
        self.assertEqual(vid[4], 102)
        self.assertEqual(vid[5], 116)
        self.assertEqual(vid[6], 121)
        self.assertEqual(vid[7], 112)
        self.assertEqual(vid[8], 105)
        self.assertEqual(vid[9], 115)
        self.assertEqual(vid[10], 111)
        self.assertEqual(vid[11], 109)

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

        response, _ = db.query(all_queries, [video_arr])

        self.assertEqual(len(response), 1)
        self.assertEqual(response[0]["AddVideo"]["status"], 0)

    def test_addVideo(self):
        db = self.create_connection()

        all_queries_to_add = []
        video_arr = []

        number_of_inserts = 2
        prefix_name = "video_"

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
            props["name"] = prefix_name + str(i)
            props["doctor"] = "Dr. Strange Love"

            video_parms = {}
            video_parms["properties"] = props
            video_parms["codec"] = "h264"

            query = {}
            query["AddVideo"] = video_parms

            all_queries_to_add.append(query)

        response_to_add, obj_to_add_array = db.query(all_queries_to_add, [video_arr])

        # Verify the videos were added by finding them
        all_queries_to_find = []

        for i in range(0, number_of_inserts):
            constraints = {}
            constraints["name"] = ["==", prefix_name + str(i)]

            video_parms = {}
            video_parms["constraints"] = constraints

            query = {}
            query["FindVideo"] = video_parms

            all_queries_to_find.append(query)

        response_to_find, vid_array_to_find = db.query(all_queries_to_find)

        self.disconnect(db)

        # Verify the results for adding the video were as expected
        self.assertEqual(len(response_to_add), number_of_inserts)
        for i in range(0, number_of_inserts):
            self.assertEqual(response_to_add[i]["AddVideo"]["status"], 0)

        # Verify the results for finding the video were as expected
        self.assertEqual(len(response_to_find), number_of_inserts)
        self.assertEqual(len(vid_array_to_find), number_of_inserts)
        for i in range(0, number_of_inserts):
            self.assertEqual(response_to_find[i]["FindVideo"]["status"], 0)

        for vid in vid_array_to_find:
            self.verify_mp4_signature(vid)

    def test_addVideoFromLocalFile_invalid_command(self):
        # The test is meant to fail if both blob and a local file are specified
        db = self.create_connection()

        with open("../test_videos/Megamind.avi", "rb") as fd:
            video_blob = fd.read()

        video_params = {}
        video_params["from_file_path"] = "BigFile.mp4"
        video_params["codec"] = "h264"

        query = {}
        query["AddVideo"] = video_params

        response, _ = db.query([query], [[video_blob]])
        self.disconnect(db)

        self.assertEqual(response[0]["status"], -1)

    def test_addVideoFromLocalFile_file_not_found(self):
        db = self.create_connection()

        video_params = {}
        video_params["from_file_path"] = "BigFile.mp4"
        video_params["codec"] = "h264"

        query = {}
        query["AddVideo"] = video_params

        response, _ = db.query([query], [[]])
        self.disconnect(db)

        self.assertEqual(response[0]["status"], -1)

    @TestCommand.TestCommand.shouldSkipRemotePythonTest()
    def test_addVideoFromLocalFile_success(self):
        db = self.create_connection()

        # Copy file to preserve the original one
        source_file = "../videos/Megamind.mp4"
        tmp_filepath = "Megamind.mp4"
        shutil.copy2(source_file, tmp_filepath)

        video_params = {}
        video_params["from_file_path"] = tmp_filepath
        video_params["codec"] = "h264"

        query = {}
        query["AddVideo"] = video_params

        response, _ = db.query([query], [[]])

        self.disconnect(db)
        if os.path.isfile(tmp_filepath) is True:
            os.remove(tmp_filepath)
        self.assertEqual(response[0]["AddVideo"]["status"], 0)

    def test_extractKeyFrames(self):
        db = self.create_connection()

        fd = open("../videos/Megamind.mp4", "rb")
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

        response, _ = db.query([query], [[video_blob]])

        self.assertEqual(response[0]["AddVideo"]["status"], 0)

        entity = {}
        entity["class"] = "VD:KF"
        entity["results"] = {"count": ""}

        query = {}
        query["FindEntity"] = entity

        response, _ = db.query([query])
        self.disconnect(db)

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
        self.disconnect(db)

        self.assertEqual(len(response), number_of_inserts)
        self.assertEqual(len(vid_array), number_of_inserts)
        for i in range(0, number_of_inserts):
            self.assertEqual(response[i]["FindVideo"]["status"], 0)

        for vid in vid_array:
            self.verify_mp4_signature(vid)

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
        self.disconnect(db)

        self.assertEqual(response[0]["FindFrames"]["status"], 0)
        self.assertEqual(response[1]["FindFrames"]["status"], 0)
        self.assertEqual(len(img_array), 2 * len(video_params["frames"]))

        for img in img_array:
            self.verify_png_signature(img)

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
        self.disconnect(db)

        self.assertEqual(response[0]["FindFrames"]["status"], 0)
        self.assertEqual(response[1]["FindFrames"]["status"], 0)
        self.assertEqual(len(img_array), 2 * number_of_frames)

        for img in img_array:
            self.verify_png_signature(img)

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
        self.disconnect(db)

        self.assertEqual(response[0]["status"], -1)
        self.assertEqual(img, [])
        self.assertEqual(
            response[0]["info"],
            "Either one of 'frames' or 'operations::interval' must be specified",
        )

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
        self.disconnect(db)

        self.assertEqual(response[0]["status"], -1)
        self.assertEqual(img, [])
        self.assertEqual(
            response[0]["info"],
            "Either one of 'frames' or 'operations::interval' must be specified",
        )

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
        self.disconnect(db)

        self.assertEqual(len(response), number_of_inserts)
        self.assertEqual(len(vid_array), number_of_inserts)
        for i in range(0, number_of_inserts):
            self.assertEqual(response[i]["FindVideo"]["status"], 0)

        for vid in vid_array:
            self.verify_mp4_signature(vid)

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

        query = {}
        query["AddVideo"] = addVideo

        all_queries.append(query)

        add_video_response, _ = db.query(all_queries, [imgs_arr])

        all_queries = []

        constraints = {}
        constraints["name"] = ["==", "Luis"]
        constraints["lastname"] = ["==", "Malo"]

        video_parms = {}
        video_parms["constraints"] = constraints

        query = {}
        query["FindVideo"] = video_parms

        all_queries.append(query)

        find_video_response, find_video_array = db.query(all_queries)

        self.disconnect(db)
        self.assertEqual(add_video_response[0]["AddEntity"]["status"], 0)
        self.assertEqual(add_video_response[1]["AddVideo"]["status"], 0)

        self.assertEqual(len(find_video_response), 1)
        self.assertEqual(len(find_video_array), 1)
        self.assertEqual(find_video_response[0]["FindVideo"]["status"], 0)
        self.verify_mp4_signature(find_video_array[0])

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
        self.disconnect(db)

        self.assertEqual(len(vid_arr), number_of_inserts)
        self.assertEqual(response[0]["FindVideo"]["status"], 0)
        self.assertEqual(response[0]["FindVideo"]["returned"], number_of_inserts)
        self.assertEqual(len(vid_arr), number_of_inserts)
        for vid in vid_arr:
            self.verify_mp4_signature(vid)

    def test_findVideoNoBlob(self):
        db = self.create_connection()

        prefix_name = "fvid_no_blob_"
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
            results["blob"] = False
            results["list"] = ["name"]

            img_params = {}
            img_params["constraints"] = constraints
            img_params["results"] = results

            query = {}
            query["FindVideo"] = img_params

            all_queries.append(query)

        response, img_array = db.query(all_queries)
        self.disconnect(db)

        for index in range(0, number_of_inserts):
            self.assertEqual(response[index]["FindVideo"]["status"], 0)
        self.assertEqual(len(img_array), 0)

    def test_updateVideo(self):
        db = self.create_connection()

        prefix_name = "fvid_update_"
        number_of_inserts = 2

        for i in range(0, number_of_inserts):
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

        # Find the updated video
        all_queries = []

        constraints = {}
        constraints["name"] = ["==", "simg_update_0"]

        video_parms = {}
        video_parms["constraints"] = constraints

        query = {}
        query["FindVideo"] = video_parms

        all_queries.append(query)

        find_response, find_vid_array = db.query(all_queries)
        self.disconnect(db)

        self.assertEqual(response[0]["UpdateVideo"]["count"], 1)
        self.assertEqual(len(img_array), 0)

        self.assertEqual(len(find_response), 1)
        self.assertEqual(len(find_vid_array), 1)
        self.assertEqual(find_response[0]["FindVideo"]["status"], 0)
        for vid in find_vid_array:
            self.verify_mp4_signature(vid)
