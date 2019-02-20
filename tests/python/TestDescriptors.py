from threading import Thread
import sys
import os
import urllib
import time
import json
import unittest
import numpy as np
import vdms # Yeah, baby

hostname = "localhost"
port = 55557

class TestDescriptors(unittest.TestCase):

    def addSet(self, name, dim, metric, engine):

        db = vdms.vdms()
        db.connect(hostname, port)

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
        db = vdms.vdms()
        db.connect(hostname, port)

        all_queries = []

        descriptor_set = {}
        descriptor_set["name"] = "features_xd"
        descriptor_set["dimensions"] = 1024*4

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

    def test_addSetAndDescriptors(self):
        db = vdms.vdms()
        db.connect(hostname, port)

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
        x = x.astype('float32')
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

    def test_addDescriptorsx1000(self):
        db = vdms.vdms()
        db.connect(hostname, port)

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

        total = 1000;

        for i in range(1,total):
            x = np.ones(dims)
            x[2] = 2.34 + i*20
            x = x.astype('float32')
            descriptor_blob.append(x.tobytes())

            descriptor = {}
            descriptor["set"] = set_name
            descriptor["label"] = "classX"

            query = {}
            query["AddDescriptor"] = descriptor

            all_queries.append(query)

        response, img_array = db.query(all_queries, [descriptor_blob])

        # Check success
        for x in range(0,total-1):
            self.assertEqual(response[x]["AddDescriptor"]["status"], 0)

    def test_addDescriptorsx1000FaissIVFFlat(self):
        db = vdms.vdms()
        db.connect(hostname, port)

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

        total = 1000;

        for i in range(1,total):
            x = np.ones(dims)
            x[2] = 2.34 + i*20
            x = x.astype('float32')
            descriptor_blob.append(x.tobytes())

            descriptor = {}
            descriptor["set"] = set_name
            descriptor["label"] = "classX"

            query = {}
            query["AddDescriptor"] = descriptor

            all_queries.append(query)

        response, img_array = db.query(all_queries, [descriptor_blob])

        # Check success
        for x in range(0,total-1):
            self.assertEqual(response[x]["AddDescriptor"]["status"], 0)


    def test_addDescriptorsx1000TileDBSparse(self):
        db = vdms.vdms()
        db.connect(hostname, port)

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

        total = 1000;

        for i in range(1,total):
            x = np.ones(dims)
            x[2] = 2.34 + i*20
            x = x.astype('float32')
            descriptor_blob.append(x.tobytes())

            descriptor = {}
            descriptor["set"] = set_name
            descriptor["label"] = "classX"

            query = {}
            query["AddDescriptor"] = descriptor

            all_queries.append(query)

        response, img_array = db.query(all_queries, [descriptor_blob])

        # Check success
        for x in range(0,total-1):
            self.assertEqual(response[x]["AddDescriptor"]["status"], 0)

    def test_addDescriptorsx1000TileDBDense(self):
        db = vdms.vdms()
        db.connect(hostname, port)

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

        total = 1000;

        for i in range(1,total):
            x = np.ones(dims)
            x[2] = 2.34 + i*20
            x = x.astype('float32')
            descriptor_blob.append(x.tobytes())

            descriptor = {}
            descriptor["set"] = set_name
            descriptor["label"] = "classX"

            query = {}
            query["AddDescriptor"] = descriptor

            all_queries.append(query)

        response, img_array = db.query(all_queries, [descriptor_blob])

        # Check success
        for x in range(0,total-1):
            self.assertEqual(response[x]["AddDescriptor"]["status"], 0)

    def test_classifyDescriptor(self):
        db = vdms.vdms()
        db.connect(hostname, port)

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

        total = 1000;

        class_counter = -1
        for i in range(0,total-1):
            if ((i % 4) == 0):
                class_counter += 1

            x = np.ones(dims)
            x[2] = 2.34 + i*20
            x = x.astype('float32')
            descriptor_blob.append(x.tobytes())

            descriptor = {}
            descriptor["set"] = set_name
            descriptor["label"] = "class" + str(class_counter)

            query = {}
            query["AddDescriptor"] = descriptor

            all_queries.append(query)

        response, img_array = db.query(all_queries, [descriptor_blob])

        # Check success
        for x in range(0,total-1):
            self.assertEqual(response[x]["AddDescriptor"]["status"], 0)


        descriptor = {}
        descriptor["set"] = set_name

        query = {}
        query["ClassifyDescriptor"] = descriptor

        for i in range(2, total//10, 4):
            all_queries = []
            descriptor_blob = []

            x = np.ones(dims)
            x[2] = 2.34 + i*20 # Calculated to be of class1
            x = x.astype('float32')
            descriptor_blob.append(x.tobytes())

            all_queries.append(query)

            response, img_array = db.query(all_queries, [descriptor_blob])

            # Check success
            self.assertEqual(response[0]["ClassifyDescriptor"]["status"], 0)
            self.assertEqual(response[0]["ClassifyDescriptor"]
                                      ["label"], "class" + str(int(i/4)))
