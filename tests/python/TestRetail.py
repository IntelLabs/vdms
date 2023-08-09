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

from threading import Thread
import TestCommand
import longquery
import numpy as np
import unittest

n_cameras = 15
dim = 1000
name = "features_vectors_store1"


class TestEntities(TestCommand.TestCommand):
    def add_descriptor_set(self, name, dim):
        db = self.create_connection()

        all_queries = []

        descriptor_set = {}
        descriptor_set["name"] = name
        descriptor_set["dimensions"] = dim

        query = {}
        query["AddDescriptorSet"] = descriptor_set

        all_queries.append(query)

        response, img_array = db.query(all_queries)

        # Check success
        self.assertEqual(response[0]["AddDescriptorSet"]["status"], 0)

    def build_store(self):
        db = self.create_connection()

        all_queries = []

        store_ref = 999

        query = {
            "AddEntity": {
                "_ref": store_ref,
                "class": "Store",
                "constraints": {"Name": ["==", "Walmart"]},
                "properties": {
                    "Address": "1428 alex way, Hillsboro 97124",
                    "Name": "Walmart",
                    "Type": "grocerys",
                },
            }
        }

        all_queries.append(query)

        areas_tag = [
            "ChildrenClothes",
            "WomenClothes",
            "MenClothes",
            "Computers",
            "Sport",
            "Food",
            "ChildrenClothes",
            "WomenClothes",
            "MenClothes",
            "Computers",
            "Sport",
            "Food",
            "ChildrenClothes",
            "ChildrenClothes",
            "WomenClothes",
            "MenClothes",
            "Computers",
            "Sport",
            "Food",
            "ChildrenClothes",
        ]

        for i in range(1, n_cameras + 1):
            addCamera = {
                "AddEntity": {
                    "_ref": i,
                    "class": "Camera",
                    "constraints": {"Name": ["==", "cam" + str(i)]},
                    "properties": {"Name": "cam" + str(i)},
                }
            }

            all_queries.append(addCamera)

            addArea = {
                "AddEntity": {
                    "_ref": n_cameras * 10 + i,
                    "class": "Area",
                    "constraints": {"Name": ["==", "Area" + str(i)]},
                    "properties": {"Name": "Area" + str(i), "Tag": areas_tag[i]},
                }
            }

            if i == 1:
                addArea["AddEntity"]["properties"]["Tag"] = "Entrance"

            if i == n_cameras:
                addArea["AddEntity"]["properties"]["Tag"] = "Exit"

            all_queries.append(addArea)

            addConnection = {
                "AddConnection": {
                    "class": "Covers",
                    "ref1": i,
                    "ref2": n_cameras * 10 + i,
                }
            }

            all_queries.append(addConnection)

            addConnection = {
                "AddConnection": {
                    "class": "Consists_Of",
                    "ref1": store_ref,
                    "ref2": n_cameras * 10 + i,
                }
            }

            all_queries.append(addConnection)

        response, res_arr = db.query(all_queries)
        # print (db.get_last_response_str())

        self.assertEqual(response[0]["AddEntity"]["status"], 0)

        for i in range(1, n_cameras + 1):
            self.assertEqual(response[(i - 1) * 4 + 1]["AddEntity"]["status"], 0)
            self.assertEqual(response[(i - 1) * 4 + 2]["AddEntity"]["status"], 0)
            self.assertEqual(response[(i - 1) * 4 + 3]["AddConnection"]["status"], 0)
            self.assertEqual(response[(i - 1) * 4 + 4]["AddConnection"]["status"], 0)

    def single(self, thID, db, results):
        # id = "19149ec8-fa0d-4ed0-9cfb-3e0811b75391"
        id = "19149ec8-fa0d-4ed0-9cfb-3e0811b" + str(thID)

        all_queries = longquery.queryPerson(id)

        # send one random fv
        descriptor_blob = []
        x = np.ones(dim)
        x[2] = 2.34 + np.random.random_sample()
        x = x.astype("float32")
        descriptor_blob.append(x.tobytes())

        try:
            response, res_arr = db.query(all_queries, [descriptor_blob])

            for i in range(0, len(response)):
                cmd = list(response[i].items())[0][0]
                self.assertEqual(response[i][cmd]["status"], 0)

            all_queries = longquery.queryVisit(id)

            response, res_arr = db.query(all_queries)

            for i in range(0, len(response)):
                cmd = list(response[i].items())[0][0]
                self.assertEqual(response[i][cmd]["status"], 0)

        except:
            results[thID] = -1

        results[thID] = 0

    @unittest.skip("Skipping class until fixed")
    def test_concurrent(self):
        self.build_store()
        self.add_descriptor_set(name, dim)

        retries = 2
        concurrency = 64

        db_list = []

        for i in range(0, concurrency):
            db = self.create_connection()
            db_list.append(db)

        results = [None] * concurrency * retries
        for ret in range(0, retries):
            thread_arr = []
            for i in range(0, concurrency):
                idx = concurrency * ret + i
                thread_add = Thread(target=self.single, args=(idx, db_list[i], results))
                thread_add.start()
                thread_arr.append(thread_add)

            idx = concurrency * ret
            error_counter = 0
            for th in thread_arr:
                th.join()
                if results[idx] == -1:
                    error_counter += 1
                idx += 1

        self.assertEqual(error_counter, 0)
