#! /usr/bin/python
from threading import Thread
import sys
import os
import urllib
import time
import json
import unittest

import athena # Yeah, baby

hostname = "localhost"

class TestStringMethods(unittest.TestCase):

    #Methos to insert one image
    def insertImage(self, props, db):

        imgs_arr = []
        all_queries = []

        fd = open("../test_images/brain.png")
        imgs_arr.append(fd.read())

        # adds some prop
        props["test_case"] = "test_case_prop"

        img_params = {}
        img_params["properties"] = props
        img_params["format"] = "png"
        img_params["log"] = True

        query = {}
        query["AddImage"] = img_params

        all_queries.append(query)

        db.query(all_queries, [imgs_arr])

    def test_addImage(self):
        db = athena.Athena()
        db.connect(hostname)

        all_queries = []
        imgs_arr = []

        number_of_inserts = 2

        for i in range(0,number_of_inserts):
            #Read Brain Image
            fd = open("../test_images/brain.png")
            imgs_arr.append(fd.read())

            collections = {}
            collections["name"] = "BrainScans"

            op_params_resize = {}
            op_params_resize["height"] = 512
            op_params_resize["width"]  = 512
            op_params_resize["type"] = "resize"

            props = {}
            props["name"] = "brain_" + str(i)
            props["doctor"] = "Dr. Strange Love"

            img_params = {}
            img_params["properties"] = props
            # img_params["collections"] = collections
            img_params["operation"] = op_params_resize
            img_params["format"] = "png"
            img_params["log"] = True

            query = {}
            query["AddImage"] = img_params

            all_queries.append(query)

        response, img_array = db.query(all_queries, [imgs_arr])
        # print athena.aux_print_json(response)

        response = json.loads(response)
        self.assertEqual(len(response), number_of_inserts)
        for i in range(0, number_of_inserts):
            self.assertEqual(response[i]["AddImage"]["status"], 0)

    def test_findEntity(self):
        db = athena.Athena()
        db.connect(hostname)

        prefix_name = "fent_brain_"

        for i in range(0,2):
            props = {}
            props["name"] = prefix_name + str(i)
            self.insertImage(props, db)

        all_queries = []

        for i in range(0,2):
            constraints = {}
            constraints["name"] = ["==", prefix_name + str(i)]

            results = {}
            results["list"] = ["name"]

            img_params = {}
            img_params["constraints"] = constraints
            img_params["results"] = results
            img_params["class"] = "AT:IMAGE"

            query = {}
            query["FindEntity"] = img_params

            all_queries.append(query)

        response, img_array = db.query(all_queries)
        # print athena.aux_print_json(response)

        response = json.loads(response)
        self.assertEqual(response[0]["FindEntity"]["status"], 0)
        self.assertEqual(response[1]["FindEntity"]["status"], 0)
        self.assertEqual(response[0]["FindEntity"]["entities"][0]["name"], prefix_name + "0")
        self.assertEqual(response[1]["FindEntity"]["entities"][0]["name"], prefix_name + "1")

    def test_findImage(self):
        db = athena.Athena()
        db.connect(hostname)

        prefix_name = "fimg_brain_"

        for i in range(0,2):
            props = {}
            props["name"] = prefix_name + str(i)
            self.insertImage(props, db)

        all_queries = []

        for i in range(0,2):
            constraints = {}
            constraints["name"] = ["==", prefix_name + str(i)]

            img_params = {}
            img_params["constraints"] = constraints
            img_params["log"] = True

            query = {}
            query["FindImage"] = img_params

            all_queries.append(query)

        response, img_array = db.query(all_queries)
        # print athena.aux_print_json(response)

        response = json.loads(response)
        self.assertEqual(response[0]["FindImage"]["status"], 0)
        self.assertEqual(response[1]["FindImage"]["status"], 0)
        # self.assertEqual(response[0]["FindImage"]["entities"][0]["name"], prefix_name + "0")
        # self.assertEqual(response[1]["FindImage"]["entities"][0]["name"], prefix_name + "1")
        self.assertEqual(len(img_array), 2)

    def test_findImage_multiple_res(self):
        db = athena.Athena()
        db.connect(hostname)

        prefix_name = "fimg_brain_multiple"

        number_of_inserts = 4
        for i in range(0,number_of_inserts):
            props = {}
            props["name"] = prefix_name
            self.insertImage(props, db)

        constraints = {}
        constraints["name"] = ["==", prefix_name]

        results = {}
        results["list"] = ["name"]

        img_params = {}
        # img_params["results"] = results
        img_params["constraints"] = constraints
        img_params["log"] = True

        query = {}
        query["FindImage"] = img_params

        all_queries = []
        all_queries.append(query)

        response, img_array = db.query(all_queries)
        # print athena.aux_print_json(response)

        response = json.loads(response)
        self.assertEqual(len(img_array), number_of_inserts)
        self.assertEqual(response[0]["FindImage"]["status"], 0)
        self.assertEqual(response[0]["FindImage"]["returned"], number_of_inserts)


if __name__ == '__main__':
    unittest.main()