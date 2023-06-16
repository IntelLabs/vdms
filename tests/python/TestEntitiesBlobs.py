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


class TestEntitiesBlob(TestCommand.TestCommand):
    def test_addEntityWithBlob(self, thID=0):
        db = self.create_connection()

        props = {}
        props["name"] = "Luis"
        props["lastname"] = "Ferro"
        props["age"] = 27
        props["threadid"] = thID

        addEntity = {}
        addEntity["properties"] = props
        addEntity["class"] = "AwesomePeople"
        addEntity["blob"] = True

        query = {}
        query["AddEntity"] = addEntity

        all_queries = []
        all_queries.append(query)

        blob_arr = []
        fd = open("../test_images/brain.png", "rb")
        blob_arr.append(fd.read())
        fd.close()

        response, res_arr = db.query(all_queries, [blob_arr])

        self.assertEqual(response[0]["AddEntity"]["status"], 0)

    def test_addEntityWithBlobNoBlob(self, thID=0):
        db = self.create_connection()

        props = {}
        props["name"] = "Luis"
        props["lastname"] = "Ferro"
        props["age"] = 27
        props["threadid"] = thID

        addEntity = {}
        addEntity["properties"] = props
        addEntity["class"] = "AwesomePeople"
        addEntity["blob"] = True

        query = {}
        query["AddEntity"] = addEntity

        all_queries = []
        all_queries.append(query)

        response, res_arr = db.query(all_queries)

        self.assertEqual(response[0]["status"], -1)
        self.assertEqual(response[0]["info"], "Expected blobs: 1. Received blobs: 0")

    def test_addEntityWithBlobAndFind(self, thID=0):
        db = self.create_connection()

        props = {}
        props["name"] = "Tom"
        props["lastname"] = "Slash"
        props["age"] = 27
        props["id"] = 45334

        addEntity = {}
        addEntity["properties"] = props
        addEntity["class"] = "NotSoAwesome"
        addEntity["blob"] = True

        query = {}
        query["AddEntity"] = addEntity

        all_queries = []
        all_queries.append(query)

        blob_arr = []
        fd = open("../test_images/brain.png", "rb")
        blob_arr.append(fd.read())
        fd.close()

        response, res_arr = db.query(all_queries, [blob_arr])

        self.assertEqual(response[0]["AddEntity"]["status"], 0)

        constraints = {}
        constraints["id"] = ["==", 45334]

        results = {}
        results["blob"] = True
        results["list"] = ["name"]

        FindEntity = {}
        FindEntity["constraints"] = constraints
        FindEntity["class"] = "NotSoAwesome"
        FindEntity["results"] = results

        query = {}
        query["FindEntity"] = FindEntity

        all_queries = []
        all_queries.append(query)

        response, res_arr = db.query(all_queries)

        self.assertEqual(response[0]["FindEntity"]["entities"][0]["blob"], True)

        self.assertEqual(len(res_arr), len(blob_arr))
        self.assertEqual(len(res_arr[0]), len(blob_arr[0]))
        self.assertEqual((res_arr[0]), (blob_arr[0]))
