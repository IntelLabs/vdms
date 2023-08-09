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


class TestBoundingBox(TestCommand.TestCommand):
    @classmethod
    def setUpClass(self):
        self.number_of_inserts = 2

    # Method to insert one bounding box
    def insertBoundingBox(self, db, props=None):
        all_queries = []
        bb = {}

        bb_coords = {}
        bb_coords["x"] = 10
        bb_coords["y"] = 10
        bb_coords["h"] = 100
        bb_coords["w"] = 100
        bb["rectangle"] = bb_coords

        # adds some prop
        if not props is None:
            bb["properties"] = props

        query = {}
        query["AddBoundingBox"] = bb

        all_queries.append(query)
        response, res_arr = db.query(all_queries)

        # Check success
        self.assertEqual(response[0]["AddBoundingBox"]["status"], 0)

    def addBoundingBoxwithImage(self, db, numBoxes, imgprops=None):
        all_queries = []
        imgs_arr = []

        fd = open("../test_images/brain.png", "rb")
        imgs_arr.append(fd.read())
        fd.close()

        img = {}
        img["properties"] = imgprops
        img["format"] = "png"
        img["_ref"] = 12

        query = {}
        query["AddImage"] = img

        all_queries.append(query)

        basename = imgprops["name"] + "_bb_"
        for x in range(0, numBoxes):
            bb_coords = {}
            bb_coords["x"] = x * 10
            bb_coords["y"] = x * 10
            bb_coords["h"] = 100
            bb_coords["w"] = 100

            bbprops = {}
            bbprops["name"] = basename + str(x)
            bb = {}
            bb["properties"] = bbprops
            bb["rectangle"] = bb_coords
            bb["image"] = 12

            bb_query = {}
            bb_query["AddBoundingBox"] = bb
            all_queries.append(bb_query)

        response, res_arr = db.query(all_queries, [imgs_arr])

        self.assertEqual(len(response), numBoxes + 1)
        self.assertEqual(response[0]["AddImage"]["status"], 0)
        for i in range(0, numBoxes):
            self.assertEqual(response[i + 1]["AddBoundingBox"]["status"], 0)

    def test_addBoundingBox(self):
        db = self.create_connection()

        all_queries = []

        for i in range(0, self.number_of_inserts):
            bb_coords = {}
            bb_coords["x"] = i
            bb_coords["y"] = i
            bb_coords["h"] = 512
            bb_coords["w"] = 512

            props = {}
            props["name"] = "my_bb_" + str(i)

            bb = {}
            bb["properties"] = props
            bb["rectangle"] = bb_coords

            query = {}
            query["AddBoundingBox"] = bb

            all_queries.append(query)

        response, img_array = db.query(all_queries)

        self.assertEqual(len(response), self.number_of_inserts)
        for i in range(0, self.number_of_inserts):
            self.assertEqual(response[i]["AddBoundingBox"]["status"], 0)

    def test_findBoundingBox(self):
        db = self.create_connection()

        prefix_name = "find_my_bb_"

        for i in range(0, self.number_of_inserts):
            props = {}
            props["name"] = prefix_name + str(i)
            self.insertBoundingBox(db, props=props)

        all_queries = []

        for i in range(0, self.number_of_inserts):
            constraints = {}
            constraints["name"] = ["==", prefix_name + str(i)]

            results = {}
            results["list"] = ["name"]

            bb_params = {}
            bb_params["constraints"] = constraints
            bb_params["results"] = results

            query = {}
            query["FindBoundingBox"] = bb_params

            all_queries.append(query)

        response, img_array = db.query(all_queries)

        self.assertEqual(response[0]["FindBoundingBox"]["status"], 0)
        self.assertEqual(response[1]["FindBoundingBox"]["status"], 0)
        self.assertEqual(
            response[0]["FindBoundingBox"]["entities"][0]["name"], prefix_name + "0"
        )
        self.assertEqual(
            response[1]["FindBoundingBox"]["entities"][0]["name"], prefix_name + "1"
        )

    def test_findBoundingBoxCoordinates(self):
        db = self.create_connection()

        prefix_name = "find_my_bb_coords_"

        for i in range(0, self.number_of_inserts):
            props = {}
            props["name"] = prefix_name + str(i)
            self.insertBoundingBox(db, props=props)

        all_queries = []

        for i in range(0, self.number_of_inserts):
            constraints = {}
            constraints["name"] = ["==", prefix_name + str(i)]

            results = {}
            results["list"] = ["_coordinates"]

            bb_params = {}
            bb_params["constraints"] = constraints
            bb_params["results"] = results

            query = {}
            query["FindBoundingBox"] = bb_params

            all_queries.append(query)

        response, img_array = db.query(all_queries)

        for i in range(0, self.number_of_inserts):
            self.assertEqual(response[i]["FindBoundingBox"]["status"], 0)
            self.assertEqual(
                response[i]["FindBoundingBox"]["entities"][0]["_coordinates"]["x"], 10
            )
            self.assertEqual(
                response[i]["FindBoundingBox"]["entities"][0]["_coordinates"]["y"], 10
            )
            self.assertEqual(
                response[i]["FindBoundingBox"]["entities"][0]["_coordinates"]["w"], 100
            )
            self.assertEqual(
                response[i]["FindBoundingBox"]["entities"][0]["_coordinates"]["h"], 100
            )

    def test_addBoundingBoxWithImage(self):
        db = self.create_connection()

        all_queries = []
        imgs_arr = []

        fd = open("../test_images/brain.png", "rb")
        imgs_arr.append(fd.read())
        fd.close()

        imgprops = {}
        imgprops["name"] = "my_brain"
        img_params = {}
        img_params["properties"] = imgprops
        img_params["format"] = "png"
        img_params["_ref"] = 12

        query = {}
        query["AddImage"] = img_params

        all_queries.append(query)

        bb_coords = {}
        bb_coords["x"] = 100
        bb_coords["y"] = 100
        bb_coords["h"] = 180
        bb_coords["w"] = 180

        props = {}
        props["name"] = "my_brain_bb"

        bb = {}
        bb["properties"] = props
        bb["rectangle"] = bb_coords
        bb["image"] = 12

        bb_query = {}
        bb_query["AddBoundingBox"] = bb

        all_queries.append(bb_query)

        response, res_arr = db.query(all_queries, [imgs_arr])

        self.assertEqual(response[0]["AddImage"]["status"], 0)
        self.assertEqual(response[1]["AddBoundingBox"]["status"], 0)

    def test_findBoundingBoxesInImage(self):
        db = self.create_connection()

        img_name = "my_brain_multiple"
        imgprops = {}
        imgprops["name"] = img_name
        self.addBoundingBoxwithImage(db, self.number_of_inserts, imgprops)

        all_queries = []

        constraints = {}
        constraints["name"] = ["==", img_name]
        img = {}
        img["constraints"] = constraints
        img["_ref"] = 42
        query = {}
        query["FindImage"] = img
        all_queries.append(query)

        results = {}
        results["list"] = ["_coordinates", "name"]

        bb_params = {}
        bb_params["image"] = 42
        bb_params["results"] = results

        query = {}
        query["FindBoundingBox"] = bb_params

        all_queries.append(query)

        response, img_array = db.query(all_queries)

        self.assertEqual(response[0]["FindImage"]["status"], 0)
        self.assertEqual(response[1]["FindBoundingBox"]["status"], 0)
        self.assertEqual(
            response[1]["FindBoundingBox"]["returned"], self.number_of_inserts
        )

        for i in range(0, self.number_of_inserts):
            ind = self.number_of_inserts - i - 1
            self.assertEqual(
                response[1]["FindBoundingBox"]["entities"][i]["_coordinates"]["x"],
                10 * ind,
            )
            self.assertEqual(
                response[1]["FindBoundingBox"]["entities"][i]["_coordinates"]["y"],
                10 * ind,
            )
            self.assertEqual(
                response[1]["FindBoundingBox"]["entities"][i]["_coordinates"]["w"], 100
            )
            self.assertEqual(
                response[1]["FindBoundingBox"]["entities"][i]["_coordinates"]["h"], 100
            )
            self.assertEqual(
                response[1]["FindBoundingBox"]["entities"][i]["name"],
                "my_brain_multiple_bb_" + str(ind),
            )

    def test_findBoundingBoxByCoordinates(self):
        db = self.create_connection()

        all_queries = []

        rect_coords = {}
        rect_coords["x"] = 0
        rect_coords["y"] = 0
        rect_coords["w"] = 500
        rect_coords["h"] = 500

        results = {}
        results["list"] = ["name"]

        bb_params = {}
        bb_params["rectangle"] = rect_coords
        bb_params["results"] = results

        query = {}
        query["FindBoundingBox"] = bb_params

        all_queries.append(query)

        response, img_array = db.query(all_queries)

        self.assertEqual(response[0]["FindBoundingBox"]["status"], 0)

    def test_findBoundingBoxBlob(self):
        db = self.create_connection()

        prefix_name = "my_brain_return_"
        all_queries = []

        for i in range(0, self.number_of_inserts):
            db = self.create_connection()

            img_name = prefix_name + str(i)
            imgprops = {}
            imgprops["name"] = img_name
            self.addBoundingBoxwithImage(db, 1, imgprops)

        for i in range(0, self.number_of_inserts):
            constraints = {}
            constraints["name"] = ["==", prefix_name + str(i) + "_bb_0"]

            results = {}
            results["blob"] = True
            results["list"] = ["name"]

            img_params = {}
            img_params["constraints"] = constraints
            img_params["results"] = results

            query = {}
            query["FindBoundingBox"] = img_params

            all_queries.append(query)

        response, img_array = db.query(all_queries)

        self.assertEqual(len(img_array), self.number_of_inserts)
        for i in range(0, self.number_of_inserts):
            coord = self.number_of_inserts - i - 1
            self.assertEqual(response[i]["FindBoundingBox"]["status"], 0)
            self.assertEqual(
                response[i]["FindBoundingBox"]["entities"][0]["name"],
                prefix_name + str(i) + "_bb_0",
            )

    def test_findBoundingBoxBlobComplex(self):
        db = self.create_connection()

        prefix_name = "my_brain_complex_"
        all_queries = []

        for i in range(0, self.number_of_inserts):
            db = self.create_connection()

            img_name = prefix_name + str(i)
            imgprops = {}
            imgprops["name"] = img_name
            self.addBoundingBoxwithImage(db, 1, imgprops)

        rect_coords = {}
        rect_coords["x"] = 0
        rect_coords["y"] = 0
        rect_coords["w"] = 500
        rect_coords["h"] = 500

        results = {}
        results["blob"] = True
        results["list"] = ["name"]

        bb_params = {}
        bb_params["rectangle"] = rect_coords
        bb_params["results"] = results

        query = {}
        query["FindBoundingBox"] = bb_params

        all_queries.append(query)

        response, img_array = db.query(all_queries)

        self.assertEqual(response[0]["FindBoundingBox"]["status"], 0)
        self.assertTrue(len(img_array) >= self.number_of_inserts)
        for i in range(0, self.number_of_inserts):
            test = {}
            test["name"] = prefix_name + str(i) + "_bb_0"
            self.assertIn(test, response[0]["FindBoundingBox"]["entities"])

    def test_updateBoundingBox(self):
        db = self.create_connection()

        prefix_name = "update_bb_"

        for i in range(0, self.number_of_inserts):
            props = {}
            props["name"] = prefix_name + str(i)
            self.insertBoundingBox(db, props=props)

        all_queries = []

        constraints = {}
        constraints["name"] = ["==", prefix_name + str(0)]

        props = {}
        props["name"] = "updated_bb_0"

        bb_params = {}
        bb_params["constraints"] = constraints
        bb_params["properties"] = props

        query = {}
        query["UpdateBoundingBox"] = bb_params

        all_queries.append(query)

        response, img_array = db.query(all_queries)

        self.assertEqual(response[0]["UpdateBoundingBox"]["status"], 0)
        self.assertEqual(response[0]["UpdateBoundingBox"]["count"], 1)

        all_queries = []
        constraints = {}
        constraints["name"] = ["==", "updated_bb_0"]

        results = {}
        results["list"] = ["name"]

        bb_params = {}
        bb_params["constraints"] = constraints
        bb_params["results"] = results

        query = {}
        query["FindBoundingBox"] = bb_params

        all_queries.append(query)

        response, img_array = db.query(all_queries)

        self.assertEqual(response[0]["FindBoundingBox"]["status"], 0)
        self.assertEqual(
            response[0]["FindBoundingBox"]["entities"][0]["name"], "updated_bb_0"
        )

    def test_updateBoundingBoxCoords(self):
        db = self.create_connection()

        prefix_name = "update_bb_"

        for i in range(0, self.number_of_inserts):
            props = {}
            props["name"] = prefix_name + str(i)
            self.insertBoundingBox(db, props=props)

        all_queries = []

        constraints = {}
        constraints["name"] = ["==", prefix_name + str(0)]

        rect_coords = {}
        rect_coords["x"] = 15
        rect_coords["y"] = 15
        rect_coords["w"] = 75
        rect_coords["h"] = 75

        bb_params = {}
        bb_params["constraints"] = constraints
        bb_params["rectangle"] = rect_coords

        query = {}
        query["UpdateBoundingBox"] = bb_params

        all_queries.append(query)

        response, img_array = db.query(all_queries)

        self.assertEqual(response[0]["UpdateBoundingBox"]["status"], 0)
        self.assertEqual(response[0]["UpdateBoundingBox"]["count"], 1)

        all_queries = []
        constraints = {}
        constraints["name"] = ["==", prefix_name + str(0)]

        results = {}
        results["list"] = ["_coordinates"]

        bb_params = {}
        bb_params["constraints"] = constraints
        bb_params["results"] = results

        query = {}
        query["FindBoundingBox"] = bb_params

        all_queries.append(query)

        response, img_array = db.query(all_queries)

        self.assertEqual(response[0]["FindBoundingBox"]["status"], 0)
        self.assertEqual(
            response[0]["FindBoundingBox"]["entities"][0]["_coordinates"]["x"], 15
        )
        self.assertEqual(
            response[0]["FindBoundingBox"]["entities"][0]["_coordinates"]["y"], 15
        )
        self.assertEqual(
            response[0]["FindBoundingBox"]["entities"][0]["_coordinates"]["w"], 75
        )
        self.assertEqual(
            response[0]["FindBoundingBox"]["entities"][0]["_coordinates"]["h"], 75
        )
