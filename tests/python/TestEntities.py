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


class TestEntities(TestCommand.TestCommand):
    def addSingleEntity(self, thID, results):
        props = {}
        props["name"] = "Luis"
        props["lastname"] = "Ferro"
        props["age"] = 27
        props["threadid"] = thID

        response, arr = self.addEntity(
            "AwesomePeople", properties=props, check_status=False
        )

        try:
            self.assertEqual(response[0]["AddEntity"]["status"], 0)
        except:
            results[thID] = -1

        results[thID] = 0

    def findEntity(self, thID, results):
        db = self.create_connection()

        constraints = {}
        constraints["threadid"] = ["==", thID]

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

        try:
            self.assertEqual(response[0]["FindEntity"]["status"], 0)
            self.assertEqual(
                response[0]["FindEntity"]["entities"][0]["lastname"], "Ferro"
            )
            self.assertEqual(response[0]["FindEntity"]["entities"][0]["threadid"], thID)
        except:
            results[thID] = -1

        results[thID] = 0

    def test_runMultipleAdds(self):
        # Test concurrent AddEntities
        concurrency = 32
        thread_arr = []
        results = [None] * concurrency
        for i in range(0, concurrency):
            thread_add = Thread(target=self.addSingleEntity, args=(i, results))
            thread_add.start()
            thread_arr.append(thread_add)

        idx = 0
        error_counter = 0
        for th in thread_arr:
            th.join()
            if results[idx] == -1:
                error_counter += 1
            idx += 1

        self.assertEqual(error_counter, 0)

        thread_arr = []

        # Tests concurrent AddEntities and FindEntities (that should exists)
        results = [None] * concurrency * 2
        for i in range(0, concurrency):
            addidx = concurrency + i
            thread_add = Thread(target=self.addSingleEntity, args=(addidx, results))
            thread_add.start()
            thread_arr.append(thread_add)

            thread_find = Thread(target=self.findEntity, args=(i, results))
            thread_find.start()
            thread_arr.append(thread_find)

        idx = 0
        error_counter = 0
        for th in thread_arr:
            th.join()
            if results[idx] == -1:
                error_counter += 1

            idx += 1

        self.assertEqual(error_counter, 0)

    def test_addFindEntity(self):
        results = [None] * 1
        self.addSingleEntity(0, results)
        self.findEntity(0, results)

    def test_addEntityWithLink(self):
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

    def test_addfindEntityWrongConstraints(self):
        db = self.create_connection()

        all_queries = []

        props = {"name": "Luis", "lastname": "Ferro", "age": 25}
        addEntity = {}
        addEntity["_ref"] = 32
        addEntity["properties"] = props
        addEntity["class"] = "SomePeople"

        query = {}
        query["AddEntity"] = addEntity

        all_queries.append(query)

        response, res_arr = db.query(all_queries)

        self.assertEqual(response[0]["AddEntity"]["status"], 0)

        all_queries = []

        # this format is invalid, as each constraint must be an array
        constraints = {"name": "Luis"}

        entity = {}
        entity["constraints"] = constraints
        entity["class"] = "SomePeople"
        entity["results"] = {"count": ""}

        query = {}
        query["FindEntity"] = entity

        all_queries.append(query)

        response, blob_arr = db.query(all_queries)

        self.assertEqual(response[0]["status"], -1)
        self.assertEqual(
            response[0]["info"], "Constraint for property 'name' must be an array"
        )

        # Another invalid format
        constraints = {"name": []}
        entity["constraints"] = constraints
        all_queries = []
        all_queries.append(query)

        response, blob_arr = db.query(all_queries)

        self.assertEqual(response[0]["status"], -1)
        self.assertEqual(
            response[0]["info"],
            "Constraint for property 'name' must be an array of size 2 or 4",
        )

    def test_FindWithSortKey(self):
        db = self.create_connection()

        all_queries = []

        number_of_inserts = 10

        for i in range(0, number_of_inserts):
            props = {}
            props["name"] = "entity_" + str(i)
            props["id"] = i

            entity = {}
            entity["properties"] = props
            entity["class"] = "Random"

            query = {}
            query["AddEntity"] = entity

            all_queries.append(query)

        response, blob_arr = db.query(all_queries)

        self.assertEqual(len(response), number_of_inserts)
        for i in range(0, number_of_inserts):
            self.assertEqual(response[i]["AddEntity"]["status"], 0)

        all_queries = []

        results = {}
        results["list"] = ["name", "id"]
        results["sort"] = "id"

        entity = {}
        entity["results"] = results
        entity["class"] = "Random"

        query = {}
        query["FindEntity"] = entity

        all_queries.append(query)

        response, blob_arr = db.query(all_queries)

        self.assertEqual(response[0]["FindEntity"]["status"], 0)
        for i in range(0, number_of_inserts):
            self.assertEqual(response[0]["FindEntity"]["entities"][i]["id"], i)

    def test_FindWithSortBlock(self):
        db = self.create_connection()

        all_queries = []

        number_of_inserts = 10

        for i in range(0, number_of_inserts):
            props = {}
            props["name"] = "entity_" + str(i)
            props["id"] = i

            entity = {}
            entity["properties"] = props
            entity["class"] = "SortBlock"

            query = {}
            query["AddEntity"] = entity

            all_queries.append(query)

        response, blob_arr = db.query(all_queries)

        self.assertEqual(len(response), number_of_inserts)
        for i in range(0, number_of_inserts):
            self.assertEqual(response[i]["AddEntity"]["status"], 0)

        all_queries = []

        sort = {}
        sort["key"] = "id"
        sort["order"] = "ascending"

        results = {}
        results["list"] = ["name", "id"]
        results["sort"] = sort

        entity = {}
        entity["results"] = results
        entity["class"] = "SortBlock"

        query = {}
        query["FindEntity"] = entity

        all_queries.append(query)

        response, blob_arr = db.query(all_queries)

        self.assertEqual(response[0]["FindEntity"]["status"], 0)
        for i in range(0, number_of_inserts):
            self.assertEqual(response[0]["FindEntity"]["entities"][i]["id"], i)

        all_queries = []

        sort = {}
        sort["key"] = "id"
        sort["order"] = "descending"

        results = {}
        results["list"] = ["name", "id"]
        results["sort"] = sort

        entity = {}
        entity["results"] = results
        entity["class"] = "SortBlock"

        query = {}
        query["FindEntity"] = entity

        all_queries.append(query)

        response, blob_arr = db.query(all_queries)

        self.assertEqual(response[0]["FindEntity"]["status"], 0)
        for i in range(0, number_of_inserts):
            self.assertEqual(
                response[0]["FindEntity"]["entities"][i]["id"],
                number_of_inserts - 1 - i,
            )
