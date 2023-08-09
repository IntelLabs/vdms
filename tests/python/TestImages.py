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


class TestImages(TestCommand.TestCommand):
    # Method to insert one image
    def insertImage(self, db, props=None, collections=None, format="png"):
        imgs_arr = []
        all_queries = []

        fd = open("../test_images/brain.png", "rb")
        imgs_arr.append(fd.read())
        fd.close()

        img_params = {}

        # adds some prop
        if not props is None:
            props["test_case"] = "test_case_prop"
            img_params["properties"] = props

        if not collections is None:
            img_params["collections"] = collections

        img_params["format"] = format

        query = {}
        query["AddImage"] = img_params

        all_queries.append(query)

        response, res_arr = db.query(all_queries, [imgs_arr])

        # Check success
        self.assertEqual(response[0]["AddImage"]["status"], 0)

    def test_addImage(self):
        db = self.create_connection()

        all_queries = []
        imgs_arr = []

        number_of_inserts = 2

        for i in range(0, number_of_inserts):
            # Read Brain Image
            fd = open("../test_images/brain.png", "rb")
            imgs_arr.append(fd.read())
            fd.close()

            op_params_resize = {}
            op_params_resize["height"] = 512
            op_params_resize["width"] = 512
            op_params_resize["type"] = "resize"

            props = {}
            props["name"] = "brain_" + str(i)
            props["doctor"] = "Dr. Strange Love"

            img_params = {}
            img_params["properties"] = props
            img_params["operations"] = [op_params_resize]
            img_params["format"] = "png"

            query = {}
            query["AddImage"] = img_params

            all_queries.append(query)

        response, img_array = db.query(all_queries, [imgs_arr])

        self.assertEqual(len(response), number_of_inserts)
        for i in range(0, number_of_inserts):
            self.assertEqual(response[i]["AddImage"]["status"], 0)

    def test_findEntityImage(self):
        db = self.create_connection()

        prefix_name = "fent_brain_"

        for i in range(0, 2):
            props = {}
            props["name"] = prefix_name + str(i)
            self.insertImage(db, props=props)

        all_queries = []

        for i in range(0, 2):
            constraints = {}
            constraints["name"] = ["==", prefix_name + str(i)]

            results = {}
            results["list"] = ["name"]

            img_params = {}
            img_params["constraints"] = constraints
            img_params["results"] = results
            img_params["class"] = "VD:IMG"

            query = {}
            query["FindEntity"] = img_params

            all_queries.append(query)

        response, img_array = db.query(all_queries)

        self.assertEqual(response[0]["FindEntity"]["status"], 0)
        self.assertEqual(response[1]["FindEntity"]["status"], 0)
        self.assertEqual(
            response[0]["FindEntity"]["entities"][0]["name"], prefix_name + "0"
        )
        self.assertEqual(
            response[1]["FindEntity"]["entities"][0]["name"], prefix_name + "1"
        )

    def test_findImage(self):
        db = self.create_connection()

        prefix_name = "fimg_brain_"

        for i in range(0, 2):
            props = {}
            props["name"] = prefix_name + str(i)
            self.insertImage(db, props=props)

        all_queries = []

        for i in range(0, 2):
            constraints = {}
            constraints["name"] = ["==", prefix_name + str(i)]

            img_params = {}
            img_params["constraints"] = constraints

            query = {}
            query["FindImage"] = img_params

            all_queries.append(query)

        response, img_array = db.query(all_queries)

        self.assertEqual(response[0]["FindImage"]["status"], 0)
        self.assertEqual(response[1]["FindImage"]["status"], 0)
        self.assertEqual(len(img_array), 2)

    def test_findImageResults(self):
        db = self.create_connection()

        prefix_name = "fimg_results_"

        for i in range(0, 2):
            props = {}
            props["name"] = prefix_name + str(i)
            self.insertImage(db, props=props)

        all_queries = []

        for i in range(0, 2):
            constraints = {}
            constraints["name"] = ["==", prefix_name + str(i)]

            results = {}
            results["list"] = ["name"]

            img_params = {}
            img_params["constraints"] = constraints
            img_params["results"] = results

            query = {}
            query["FindImage"] = img_params

            all_queries.append(query)

        response, img_array = db.query(all_queries)

        self.assertEqual(response[0]["FindImage"]["status"], 0)
        self.assertEqual(response[1]["FindImage"]["status"], 0)
        self.assertEqual(
            response[0]["FindImage"]["entities"][0]["name"], prefix_name + "0"
        )
        self.assertEqual(
            response[1]["FindImage"]["entities"][0]["name"], prefix_name + "1"
        )
        self.assertEqual(len(img_array), 2)

    def test_addImageWithLink(self):
        db = self.create_connection()

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

        addImage = {}
        addImage["properties"] = props
        addImage["link"] = link
        addImage["format"] = "png"

        imgs_arr = []

        fd = open("../test_images/brain.png", "rb")
        imgs_arr.append(fd.read())
        fd.close()

        img_params = {}

        query = {}
        query["AddImage"] = addImage

        all_queries.append(query)

        response, res_arr = db.query(all_queries, [imgs_arr])

        self.assertEqual(response[0]["AddEntity"]["status"], 0)
        self.assertEqual(response[1]["AddImage"]["status"], 0)

    def test_findImage_multiple_results(self):
        db = self.create_connection()

        prefix_name = "fimg_brain_multiple"

        number_of_inserts = 4
        for i in range(0, number_of_inserts):
            props = {}
            props["name"] = prefix_name
            self.insertImage(db, props=props)

        constraints = {}
        constraints["name"] = ["==", prefix_name]

        results = {}
        results["list"] = ["name"]

        img_params = {}
        img_params["constraints"] = constraints

        query = {}
        query["FindImage"] = img_params

        all_queries = []
        all_queries.append(query)

        response, img_array = db.query(all_queries)

        self.assertEqual(len(img_array), number_of_inserts)
        self.assertEqual(response[0]["FindImage"]["status"], 0)
        self.assertEqual(response[0]["FindImage"]["returned"], number_of_inserts)

    def test_findImageNoBlob(self):
        db = self.create_connection()

        prefix_name = "fimg_no_blob_"

        for i in range(0, 2):
            props = {}
            props["name"] = prefix_name + str(i)
            self.insertImage(db, props=props)

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
            query["FindImage"] = img_params

            all_queries.append(query)

        response, img_array = db.query(all_queries)

        self.assertEqual(response[0]["FindImage"]["status"], 0)
        self.assertEqual(response[1]["FindImage"]["status"], 0)
        self.assertEqual(len(img_array), 0)

    def test_findImageRefNoBlobNoPropsResults(self):
        db = self.create_connection()

        prefix_name = "fimg_no_blob_no_res"

        for i in range(0, 2):
            props = {}
            props["name"] = prefix_name + str(i)
            props["id"] = i
            self.insertImage(db, props=props)

        all_queries = []

        for i in range(0, 1):
            constraints = {}
            constraints["name"] = ["==", prefix_name + str(i)]

            results = {}
            results["blob"] = False
            # results["list"] = ["name", "id"]

            img_params = {}
            img_params["constraints"] = constraints
            img_params["results"] = results
            img_params["_ref"] = 22

            query = {}
            query["FindImage"] = img_params

            all_queries.append(query)

        response, img_array = db.query(all_queries)
        # print(db.get_last_response_str())

        self.assertEqual(response[0]["FindImage"]["status"], 0)
        self.assertEqual(len(img_array), 0)

    def test_updateImage(self):
        db = self.create_connection()

        prefix_name = "fimg_update_"

        for i in range(0, 2):
            props = {}
            props["name"] = prefix_name + str(i)
            self.insertImage(db, props=props)

        all_queries = []

        constraints = {}
        constraints["name"] = ["==", prefix_name + str(0)]

        props = {}
        props["name"] = "simg_update_0"

        img_params = {}
        img_params["constraints"] = constraints
        img_params["properties"] = props

        query = {}
        query["UpdateImage"] = img_params

        all_queries.append(query)

        response, img_array = db.query(all_queries)

        self.assertEqual(response[0]["UpdateImage"]["count"], 1)
        self.assertEqual(len(img_array), 0)

    def ztest_zFindImageWithCollection(self):
        db = self.create_connection()

        prefix_name = "fimg_brain_collection_"
        number_of_inserts = 4

        colls = {}
        colls = ["brainScans"]

        for i in range(0, number_of_inserts):
            props = {}
            props["name"] = prefix_name + str(i)

            self.insertImage(db, props=props, collections=colls)

        all_queries = []

        for i in range(0, 1):
            results = {}
            results["list"] = ["name"]

            img_params = {}
            img_params["collections"] = ["brainScans"]
            img_params["results"] = results

            query = {}
            query["FindImage"] = img_params

            all_queries.append(query)

        response, img_array = db.query(all_queries)

        self.assertEqual(response[0]["FindImage"]["status"], 0)
        self.assertEqual(len(img_array), number_of_inserts)
