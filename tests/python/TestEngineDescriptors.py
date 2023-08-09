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

    def test_addDifferentSets(self):
        self.addSet("128-L2-FaissFlat", 128, "L2", "FaissFlat")
        self.addSet("128-IP-FaissFlat", 128, "IP", "FaissFlat")
        self.addSet("128-L2-FaissIVFFlat", 128, "L2", "FaissIVFFlat")
        self.addSet("128-IP-FaissIVFFlat", 128, "IP", "FaissIVFFlat")
        self.addSet("128-L2-TileDBDense", 128, "L2", "TileDBDense")
        self.addSet("128-L2-TileDBSparse", 128, "L2", "TileDBSparse")

        self.addSet("4075-L2-FaissFlat", 4075, "L2", "FaissFlat")
        self.addSet("4075-IP-FaissFlat", 4075, "IP", "FaissFlat")
        self.addSet("4075-L2-FaissIVFFlat", 4075, "L2", "FaissIVFFlat")
        self.addSet("4075-IP-FaissIVFFlat", 4075, "IP", "FaissIVFFlat")
        self.addSet("4075-L2-TileDBDense", 4075, "L2", "TileDBDense")

    def test_addDescriptorsx1000FaissIVFFlat(self):
        db = self.create_connection()

        all_queries = []

        # Add Set
        set_name = "faissivfflat_ip_128dx1000"
        dims = 128
        descriptor_set = {}
        descriptor_set["name"] = set_name
        descriptor_set["dimensions"] = dims
        descriptor_set["metric"] = "IP"
        descriptor_set["engine"] = "FaissIVFFlat"

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

    def test_addDescriptorsx1000TileDBSparse(self):
        db = self.create_connection()

        all_queries = []

        # Add Set
        set_name = "tiledbsparse_l2_128dx1000"
        dims = 128
        descriptor_set = {}
        descriptor_set["name"] = set_name
        descriptor_set["dimensions"] = dims
        descriptor_set["metric"] = "L2"
        descriptor_set["engine"] = "TileDBSparse"

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

    def test_addDescriptorsx1000TileDBDense(self):
        db = self.create_connection()

        all_queries = []

        # Add Set
        set_name = "tiledbdense_l2_128dx1000"
        dims = 128
        descriptor_set = {}
        descriptor_set["name"] = set_name
        descriptor_set["dimensions"] = dims
        descriptor_set["metric"] = "L2"
        descriptor_set["engine"] = "TileDBDense"

        query = {}
        query["AddDescriptorSet"] = descriptor_set

        all_queries.append(query)

        response, img_array = db.query(all_queries)
        # print(json.dumps(all_queries, indent=4, sort_keys=False))
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
