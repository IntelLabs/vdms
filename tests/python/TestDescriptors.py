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

    def test_addSet(self):
        db = vdms.VDMS()
        db.connect(hostname, port)

        all_queries = []

        descriptor_set = {}
        descriptor_set["name"] = "features_xd"
        descriptor_set["dimensions"] = 1024*4

        query = {}
        query["AddDescriptorSet"] = descriptor_set

        all_queries.append(query)

        # print vdms.aux_print_json(json.dumps(all_queries))
        response, img_array = db.query(all_queries)

        # Check success
        response = json.loads(response)
        # print vdms.aux_print_json(response)
        self.assertEqual(response[0]["AddDescriptorSet"]["status"], 0)

    def test_addSetAndDescriptors(self):
        db = vdms.VDMS()
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
        response = json.loads(response)
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
        response = json.loads(response)
        # print vdms.aux_print_json(response)
        self.assertEqual(response[0]["AddDescriptor"]["status"], 0)

    def test_addDescriptorsx1000(self):
        db = vdms.VDMS()
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
        response = json.loads(response)
        # print vdms.aux_print_json(response)
        self.assertEqual(response[0]["AddDescriptorSet"]["status"], 0)

        all_queries = []
        descriptor_blob = []

        total = 1000;

        for i in xrange(1,total):
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
        response = json.loads(response)
        # print vdms.aux_print_json(response)
        for x in xrange(0,total-1):
            self.assertEqual(response[x]["AddDescriptor"]["status"], 0)


    def test_classifyDescriptor(self):
        db = vdms.VDMS()
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
        response = json.loads(response)
        self.assertEqual(response[0]["AddDescriptorSet"]["status"], 0)

        all_queries = []
        descriptor_blob = []

        total = 1000;

        class_counter = -1
        for i in xrange(0,total-1):
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
        response = json.loads(response)
        # print vdms.aux_print_json(response)
        for x in xrange(0,total-1):
            self.assertEqual(response[x]["AddDescriptor"]["status"], 0)


        descriptor = {}
        descriptor["set"] = set_name

        query = {}
        query["ClassifyDescriptor"] = descriptor

        for i in xrange(2, total/10, 4):
            all_queries = []
            descriptor_blob = []

            x = np.ones(dims)
            x[2] = 2.34 + i*20 # Calculated to be of class1
            x = x.astype('float32')
            descriptor_blob.append(x.tobytes())

            all_queries.append(query)

            response, img_array = db.query(all_queries, [descriptor_blob])

            # Check success
            response = json.loads(response)
            # print vdms.aux_print_json(response)
            self.assertEqual(response[0]["ClassifyDescriptor"]["status"], 0)
            self.assertEqual(response[0]["ClassifyDescriptor"]
                                      ["label"], "class" + str(int(i/4)))
