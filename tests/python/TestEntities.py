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
import sys
import os
import urllib
import time
import json
import unittest
import numpy as np
import vdms

hostname = "localhost"
port = 55557

class TestEntities(unittest.TestCase):

    def addEntity(self, thID=0):

        db = vdms.vdms()
        db.connect(hostname, port)

        props = {}
        props["name"] = "Luis"
        props["lastname"] = "Ferro"
        props["age"] = 27
        props["threadid"] = thID

        addEntity = {}
        addEntity["properties"] = props
        addEntity["class"] = "AwesomePeople"

        query = {}
        query["AddEntity"] = addEntity

        all_queries = []
        all_queries.append(query)

        response, res_arr = db.query(all_queries)
        # print (db.get_last_response_str())

        self.assertEqual(response[0]["AddEntity"]["status"], 0)

    def findEntity(self, thID):

        db = vdms.vdms()
        db.connect(hostname, port)

        constraints = {}
        constraints["threadid"] = ["==",thID]

        findEntity = {}
        findEntity["constraints"] = constraints
        findEntity["class"] = "AwesomePeople"

        results = {}
        results["list"] = ["name", "lastname", "threadid"]
        findEntity["results"] = results

        query = {}
        query["FindEntity"] = findEntity

        all_queries = []
        all_queries.append(query)

        response, res_arr = db.query(all_queries)

        self.assertEqual(response[0]["FindEntity"]["status"], 0)
        self.assertEqual(response[0]["FindEntity"]["entities"][0]
                                    ["lastname"], "Ferro")
        self.assertEqual(response[0]["FindEntity"]["entities"][0]
                                    ["threadid"], thID)

    def ztest_runMultipleAdds(self):

        simultaneous = 1000;
        thread_arr = []
        for i in range(1,simultaneous):
            thread_add = Thread(target=self.addEntity,args=(i,) )
            thread_add.start()
            thread_arr.append(thread_add)
            time.sleep(0.002)

        for i in range(1,simultaneous):
            thread_find = Thread(target=self.findEntity,args=(i,) )
            thread_find.start()
            thread_arr.append(thread_find)

        for th in thread_arr:
            th.join();

    def test_addFindEntity(self):
        self.addEntity(9000);
        self.findEntity(9000);

    def test_addEntityWithLink(self):
        db = vdms.vdms()
        db.connect(hostname, port)

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
        props["lastname"] = "Bueno"
        props["age"] = 27

        link = {}
        link["ref"] = 32
        link["direction"] = "in"
        link["class"] = "Friends"

        addEntity = {}
        addEntity["properties"] = props
        addEntity["class"] = "AwesomePeople"
        addEntity["link"] = link

        img_params = {}

        query = {}
        query["AddEntity"] = addEntity

        all_queries.append(query)

        response, res_arr = db.query(all_queries)

        self.assertEqual(response[0]["AddEntity"]["status"], 0)
        self.assertEqual(response[1]["AddEntity"]["status"], 0)
