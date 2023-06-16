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
import numpy as np


class TestDescriptors(TestCommand.TestCommand):
    def addSet(self, name, dim, metric, engine):
        db = self.create_connection()

        all_queries = []

        descriptor_set = {}
        descriptor_set["name"] = name
        descriptor_set["dimensions"] = dim
        descriptor_set["metric"] = metric
        descriptor_set["engine"] = engine

        query = {}
        query["AddDescriptorSet"] = descriptor_set

        all_queries.append(query)

        response, img_array = db.query(all_queries)

        # Check success
        self.assertEqual(response[0]["AddDescriptorSet"]["status"], 0)

    def test_addSet(self):
        db = self.create_connection()

        all_queries = []

        descriptor_set = {}
        descriptor_set["name"] = "features_xd"
        descriptor_set["dimensions"] = 1024 * 4

        query = {}
        query["AddDescriptorSet"] = descriptor_set

        all_queries.append(query)

        response, img_array = db.query(all_queries)

        # Check success
        self.assertEqual(response[0]["AddDescriptorSet"]["status"], 0)

    def test_addSetAndDescriptors(self):
        db = self.create_connection()

        all_queries = []

        # Add Set
        set_name = "features_128d"
        dims = 1024
        descriptor_set = {}
        descriptor_set["name"] = set_name
        descriptor_set["dimensions"] = dims

        query = {}
        query["AddDescriptorSet"] = descriptor_set

        all_queries.append(query)

        response, img_array = db.query(all_queries)
        self.assertEqual(response[0]["AddDescriptorSet"]["status"], 0)

        # Add Descriptors
        all_queries = []
        descriptor_blob = []

        x = np.zeros(dims)
        x = x.astype("float32")
        # print type(x[0])
        # print "size: ", len(x.tobytes())/4
        descriptor_blob.append(x.tobytes())

        descriptor = {}
        descriptor["set"] = set_name

        query = {}
        query["AddDescriptor"] = descriptor

        all_queries.append(query)

        response, img_array = db.query(all_queries, [descriptor_blob])

        # Check success
        self.assertEqual(response[0]["AddDescriptor"]["status"], 0)

    def test_addSetAndDescriptorsDimMismatch(self):
        db = self.create_connection()

        all_queries = []

        # Add Set
        set_name = "features_64d_dim_mismatched"
        dims = 64
        descriptor_set = {}
        descriptor_set["name"] = set_name
        descriptor_set["dimensions"] = dims

        query = {}
        query["AddDescriptorSet"] = descriptor_set

        all_queries.append(query)

        response, img_array = db.query(all_queries)
        self.assertEqual(response[0]["AddDescriptorSet"]["status"], 0)

        # Add Descriptors
        all_queries = []
        descriptor_blob = []

        x = np.zeros(dims // 2)
        x = x.astype("float32")
        # print type(x[0])
        # print "size: ", len(x.tobytes())/4
        descriptor_blob.append(x.tobytes())

        descriptor = {}
        descriptor["set"] = set_name

        query = {}
        query["AddDescriptor"] = descriptor

        all_queries.append(query)

        response, img_array = db.query(all_queries, [descriptor_blob])

        # Check success
        self.assertEqual(response[0]["status"], -1)
        self.assertEqual(response[0]["info"], "Blob Dimensions Mismatch")

        # Add Descriptors
        all_queries = []
        descriptor_blob = []

        x = np.zeros(dims)[:-1]
        x = x.astype("float32")
        # print type(x[0])
        # print "size: ", len(x.tobytes())/4
        descriptor_blob.append(x.tobytes())

        descriptor = {}
        descriptor["set"] = set_name

        query = {}
        query["AddDescriptor"] = descriptor

        all_queries.append(query)

        response, img_array = db.query(all_queries, [descriptor_blob])

        # Check success
        self.assertEqual(response[0]["status"], -1)
        self.assertEqual(response[0]["info"], "Blob Dimensions Mismatch")

    def test_addDescriptorsx1000(self):
        db = self.create_connection()

        all_queries = []

        # Add Set
        set_name = "features_128dx1000"
        dims = 128
        descriptor_set = {}
        descriptor_set["name"] = set_name
        descriptor_set["dimensions"] = dims

        query = {}
        query["AddDescriptorSet"] = descriptor_set

        all_queries.append(query)

        response, img_array = db.query(all_queries)
        self.assertEqual(response[0]["AddDescriptorSet"]["status"], 0)

        all_queries = []
        descriptor_blob = []

        total = 2

        for i in range(1, total):
            x = np.ones(dims)
            x[2] = 2.34 + i * 20
            x = x.astype("float32")
            descriptor_blob.append(x.tobytes())

            descriptor = {}
            descriptor["set"] = set_name
            descriptor["label"] = "classX"

            query = {}
            query["AddDescriptor"] = descriptor

            all_queries.append(query)

        response, img_array = db.query(all_queries, [descriptor_blob])

        # Check success
        for x in range(0, total - 1):
            self.assertEqual(response[x]["AddDescriptor"]["status"], 0)

    def test_classifyDescriptor(self):
        db = self.create_connection()

        all_queries = []

        # Add Set
        set_name = "features_128d_4_classify"
        dims = 128
        descriptor_set = {}
        descriptor_set["name"] = set_name
        descriptor_set["dimensions"] = dims

        query = {}
        query["AddDescriptorSet"] = descriptor_set

        all_queries.append(query)

        response, img_array = db.query(all_queries)
        self.assertEqual(response[0]["AddDescriptorSet"]["status"], 0)

        all_queries = []
        descriptor_blob = []

        total = 2

        class_counter = -1
        for i in range(0, total - 1):
            if (i % 4) == 0:
                class_counter += 1

            x = np.ones(dims)
            x[2] = 2.34 + i * 20
            x = x.astype("float32")
            descriptor_blob.append(x.tobytes())

            descriptor = {}
            descriptor["set"] = set_name
            descriptor["label"] = "class" + str(class_counter)

            query = {}
            query["AddDescriptor"] = descriptor

            all_queries.append(query)

        response, img_array = db.query(all_queries, [descriptor_blob])

        # Check success
        for x in range(0, total - 1):
            self.assertEqual(response[x]["AddDescriptor"]["status"], 0)

        descriptor = {}
        descriptor["set"] = set_name

        query = {}
        query["ClassifyDescriptor"] = descriptor

        for i in range(2, total // 10, 4):
            all_queries = []
            descriptor_blob = []

            x = np.ones(dims)
            x[2] = 2.34 + i * 20  # Calculated to be of class1
            x = x.astype("float32")
            descriptor_blob.append(x.tobytes())

            all_queries.append(query)

            response, img_array = db.query(all_queries, [descriptor_blob])

            # Check success
            self.assertEqual(response[0]["ClassifyDescriptor"]["status"], 0)
            self.assertEqual(
                response[0]["ClassifyDescriptor"]["label"], "class" + str(int(i / 4))
            )
