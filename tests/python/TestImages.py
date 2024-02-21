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

import unittest
import TestCommand


class TestImages(TestCommand.TestCommand):
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

        response, _ = db.query(all_queries, [imgs_arr])

        # Check success
        self.assertEqual(response[0]["AddImage"]["status"], 0)

    def test_addImage(self):
        # Setup
        db = self.create_connection()

        all_insert_queries = []
        all_find_queries = []
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

            all_insert_queries.append(query)

        # Execute the test
        response_from_insert, _ = db.query(all_insert_queries, [imgs_arr])

        # Call to function in charge of checking the images were found
        for i in range(0, number_of_inserts):
            constraints = {}
            constraints["name"] = ["==", "brain_" + str(i)]

            img_params = {}
            img_params["constraints"] = constraints

            query = {}
            query["FindImage"] = img_params

            all_find_queries.append(query)

        response_from_find, img_found_array = db.query(all_find_queries)

        # Disconnect the connection to avoid unused connections in case of any
        # assert fails
        self.disconnect(db)

        # Verify the results
        self.assertEqual(len(response_from_insert), number_of_inserts)

        for i in range(0, number_of_inserts):
            self.assertEqual(response_from_insert[i]["AddImage"]["status"], 0)

        # Verify the images were inserted earlier, now they could be found
        for i in range(0, number_of_inserts):
            self.assertEqual(response_from_find[i]["FindImage"]["status"], 0)

        self.assertEqual(len(img_found_array), number_of_inserts)

        # Verify the blob data returned is an array of PNG data
        for img in img_found_array:
            self.verify_png_signature(img)

    def test_findEntityImage(self):
        db = self.create_connection()

        prefix_name = "fent_brain_"
        number_of_iterations = 2

        for i in range(0, number_of_iterations):
            props = {}
            props["name"] = prefix_name + str(i)
            self.insertImage(db, props=props)

        all_queries = []

        for i in range(0, number_of_iterations):
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

        response, _ = db.query(all_queries)

        for index in range(0, number_of_iterations):
            self.assertEqual(response[index]["FindEntity"]["status"], 0)

            self.assertEqual(
                response[index]["FindEntity"]["entities"][0]["name"],
                prefix_name + str(index),
            )

    def test_findImage(self):
        # Setup
        db = self.create_connection()

        prefix_name = "fimg_brain_"
        num_images = 2
        filenames = []
        for i in range(0, num_images):
            filenames.append(prefix_name + str(i))

        for i in range(0, num_images):
            props = {}
            props["name"] = filenames[i]
            self.insertImage(db, props=props)

        # Execute the tests
        all_queries = []

        for i in range(0, num_images):
            constraints = {}
            constraints["name"] = ["==", filenames[i]]

            img_params = {}
            img_params["constraints"] = constraints

            query = {}
            query["FindImage"] = img_params

            all_queries.append(query)

        response, img_array = db.query(all_queries)

        self.disconnect(db)

        # Verify the results
        for i in range(0, num_images):
            self.assertEqual(response[i]["FindImage"]["status"], 0)

        self.assertEqual(len(img_array), num_images)

        # Verify the returned blob data is an array of PNG data
        for img in img_array:
            self.verify_png_signature(img)

    def test_findImageResults(self):
        # Setup
        db = self.create_connection()

        prefix_name = "fimg_results_"
        number_of_iterations = 2

        for i in range(0, number_of_iterations):
            props = {}
            props["name"] = prefix_name + str(i)
            self.insertImage(db, props=props)

        # Execute the tests
        all_queries = []

        for i in range(0, number_of_iterations):
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

        self.disconnect(db)

        # Verify the results
        for index in range(0, number_of_iterations):
            self.assertEqual(response[index]["FindImage"]["status"], 0)
            self.assertEqual(
                response[index]["FindImage"]["entities"][0]["name"],
                prefix_name + str(index),
            )
        self.assertEqual(len(img_array), number_of_iterations)

        # Verify the returned blob data is an array of PNG data
        for img in img_array:
            self.verify_png_signature(img)

    def test_addImageWithLink(self):
        # Setup
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

        query = {}
        query["AddImage"] = addImage

        all_queries.append(query)

        # Execute the test
        response, _ = db.query(all_queries, [imgs_arr])
        self.disconnect(db)

        # Verify the results
        self.assertEqual(response[0]["AddEntity"]["status"], 0)
        self.assertEqual(response[1]["AddImage"]["status"], 0)

    def test_findImage_multiple_results(self):
        # Setup
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

        # Execute the tests
        response, img_array = db.query(all_queries)
        self.disconnect(db)

        # Verify the results
        self.assertEqual(len(img_array), number_of_inserts)
        self.assertEqual(response[0]["FindImage"]["status"], 0)
        self.assertEqual(response[0]["FindImage"]["returned"], number_of_inserts)

        # Verify the blob data returned is an array of PNG data
        for img in img_array:
            self.verify_png_signature(img)

    def test_findImageNoBlob(self):
        # Setup
        db = self.create_connection()

        prefix_name = "fimg_no_blob_"
        number_of_images = 2

        for i in range(0, number_of_images):
            props = {}
            props["name"] = prefix_name + str(i)
            self.insertImage(db, props=props)

        all_queries = []

        for i in range(0, number_of_images):
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

        # Execute the tests
        response, img_array = db.query(all_queries)
        self.disconnect(db)

        # Verify the results
        for index in range(0, number_of_images):
            self.assertEqual(response[index]["FindImage"]["status"], 0)
            self.assertEqual(
                response[index]["FindImage"]["entities"][0]["name"],
                prefix_name + str(index),
            )

        self.assertEqual(len(img_array), 0)

    def test_findImageRefNoBlobNoPropsResults(self):
        # Setup
        db = self.create_connection()

        prefix_name = "fimg_no_blob_no_res"
        expected_info = "No entities found"
        number_of_images = 2

        for i in range(0, number_of_images):
            props = {}
            props["name"] = prefix_name + str(i)
            props["id"] = i
            self.insertImage(db, props=props)

        all_queries = []

        for i in range(0, number_of_images):
            constraints = {}
            constraints["name"] = ["==", prefix_name + str(i)]

            results = {}
            results["blob"] = False
            # results["list"] = ["name", "id"]

            img_params = {}
            img_params["constraints"] = constraints
            img_params["results"] = results
            img_params["_ref"] = 22 + i

            query = {}
            query["FindImage"] = img_params

            all_queries.append(query)
        # Execute the tests
        response, img_array = db.query(all_queries)
        self.disconnect(db)
        # print(db.get_last_response_str())

        # Verify the results
        for index in range(0, number_of_images):
            self.assertEqual(response[index]["FindImage"]["status"], 0)
            self.assertEqual(response[index]["FindImage"]["info"], expected_info)
        self.assertEqual(len(img_array), 0)

    def test_updateImage(self):
        # Setup
        db = self.create_connection()

        prefix_name = "fimg_update_"
        number_of_images = 2

        for i in range(0, number_of_images):
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

        # Execute the tests
        response, img_array = db.query(all_queries)
        self.disconnect(db)

        # Verify the results
        self.assertEqual(response[0]["UpdateImage"]["count"], 1)
        self.assertEqual(response[0]["UpdateImage"]["status"], 0)
        self.assertEqual(len(img_array), 0)

    # The following test fails:
    # Error: "Object contains a property that could not be validated using
    # 'properties' or 'additionalProperties' constraints: 'AddImage'"
    @unittest.skip("Skipping the test until it is fixed")
    def test_zFindImageWithCollection(self):
        # Setup
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

        # Execute the tests
        response, img_array = db.query(all_queries)
        self.disconnect(db)

        # Verify the results
        self.assertEqual(response[0]["FindImage"]["status"], 0)
        self.assertEqual(len(img_array), number_of_inserts)
