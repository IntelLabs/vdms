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
    def create_image(
        self,
        command_str,
        ref=None,
        format=None,
        props=None,
        ops=None,
        constraints=None,
        unique=False,
        results=None,
        link=None,
        collections=None,
    ):
        entity = {}

        if ref is not None:
            entity["_ref"] = ref

        if format is not None and command_str == "AddImage":
            entity["format"] = format

        if unique and command_str == "FindImage":
            entity["unique"] = unique

        if results is not None and command_str == "FindImage":
            entity["results"] = results

        if constraints is not None:
            entity["constraints"] = constraints

        if link is not None:
            entity["link"] = link

        if collections is not None:
            entity["collections"] = collections

        if ops not in [None, {}, []]:
            entity["operations"] = ops

        if props not in [None, {}, []] and command_str in ["AddImage", "UpdateImage"]:
            entity["properties"] = props

        query = {command_str: entity}
        return query

    # Method to insert one image
    def insertImage(self, db, props=None, collections=None, format="png"):
        imgs_arr = []
        all_queries = []

        fd = open("../test_images/brain.png", "rb")
        imgs_arr.append(fd.read())
        fd.close()

        # adds some prop
        if not props is None:
            props["test_case"] = "test_case_prop"

        op_params_resize = {}
        op_params_resize["height"] = 512
        op_params_resize["width"] = 512
        op_params_resize["type"] = "resize"
        query = self.create_image(
            "AddImage",
            ref=12,
            format=format,
            ops=[op_params_resize],
            props=props,
            collections=collections,
        )

        all_queries.append(query)

        response, _ = db.query(all_queries, [imgs_arr])

        # Check success
        self.assertEqual(response[0]["AddImage"]["status"], 0)

    def test_JPG_addImage_Without_operations(self):
        db = self.create_connection()
        all_queries = []
        imgs_arr = []

        number_of_inserts = 2

        for i in range(0, number_of_inserts):
            # Read Brain Image
            fd = open("../test_images/large1.jpg", "rb")
            imgs_arr.append(fd.read())
            fd.close()

            props = {}
            props["name"] = "brain_" + str(i)
            props["doctor"] = "Dr. Strange Love"
            query = self.create_image(
                "AddImage",
                format="jpg",
                props=props,
            )

            all_queries.append(query)

        response, img_array = db.query(all_queries, [imgs_arr])

        self.assertEqual(len(response), number_of_inserts)
        for i in range(0, number_of_inserts):
            self.assertEqual(response[i]["AddImage"]["status"], 0)

    def test_PNG_addImage_Without_operations(self):
        db = self.create_connection()

        all_queries = []
        imgs_arr = []

        number_of_inserts = 2

        for i in range(0, number_of_inserts):
            # Read Brain Image
            fd = open("../test_images/brain.png", "rb")
            imgs_arr.append(fd.read())
            fd.close()

            props = {}
            props["name"] = "brain_" + str(i)
            props["doctor"] = "Dr. Strange Love"

            query = self.create_image(
                "AddImage",
                format="png",
                props=props,
            )
            all_queries.append(query)

        response, img_array = db.query(all_queries, [imgs_arr])
        self.assertEqual(len(response), number_of_inserts)
        for i in range(0, number_of_inserts):
            self.assertEqual(response[i]["AddImage"]["status"], 0)
        for img in img_array:
            self.verify_png_signature(img)

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
            props["name"] = "test_brain_" + str(i)
            props["doctor"] = "Dr. Strange Love"
            query = self.create_image(
                "AddImage",
                format="png",
                props=props,
                ops=[op_params_resize],
            )

            all_insert_queries.append(query)

        # Execute the test
        response_from_insert, _ = db.query(all_insert_queries, [imgs_arr])

        # Call to function in charge of checking the images were found
        for i in range(0, number_of_inserts):
            constraints = {}
            constraints["name"] = ["==", "test_brain_" + str(i)]

            query = self.create_image(
                "FindImage",
                constraints=constraints,
            )
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

            query = self.create_entity(
                "FindEntity",
                class_str="VD:IMG",
                results=results,
                constraints=constraints,
            )
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

            query = self.create_image(
                "FindImage",
                constraints=constraints,
            )
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

            query = self.create_image(
                "FindImage", constraints=constraints, results=results
            )
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

        query = self.create_entity(
            "AddEntity", class_str="AwesomePeople", ref=32, props=props
        )
        all_queries.append(query)

        props = {}
        props["name"] = "Luis"
        props["lastname"] = "Malo"
        props["age"] = 27

        link = {}
        link["ref"] = 32
        link["direction"] = "in"
        link["class"] = "Friends"

        imgs_arr = []

        fd = open("../test_images/brain.png", "rb")
        imgs_arr.append(fd.read())
        fd.close()

        query = self.create_image(
            "AddImage",
            format="png",
            link=link,
            props=props,
        )
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

        query = self.create_image("FindImage", constraints=constraints, results=results)
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

            query = self.create_image(
                "FindImage", constraints=constraints, results=results
            )
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

            query = self.create_image(
                "FindImage", ref=22 + i, constraints=constraints, results=results
            )
            all_queries.append(query)
        # Execute the tests
        response, img_array = db.query(all_queries)
        self.disconnect(db)

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

        query = self.create_image("UpdateImage", constraints=constraints, props=props)
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

            query = self.create_image(
                "FindImage", collections=["brainScans"], results=results
            )
            all_queries.append(query)

        # Execute the tests
        response, img_array = db.query(all_queries)
        self.disconnect(db)

        # Verify the results
        self.assertEqual(response[0]["FindImage"]["status"], 0)
        self.assertEqual(len(img_array), number_of_inserts)
