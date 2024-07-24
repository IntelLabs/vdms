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
    def addSingleEntity(self, thID, results, db):
        props = {}
        props["name"] = "Luis"
        props["lastname"] = "Ferro"
        props["age"] = 27
        props["threadid"] = thID

        response, arr = self.addEntity(
            "AwesomePeople", db=db, properties=props, check_status=False
        )

        try:
            self.assertEqual(response[0]["AddEntity"]["status"], 0)
        except:
            results[thID] = -1

        results[thID] = 0

    def findEntity(self, thID, results, db):
        constraints = {}
        constraints["threadid"] = ["==", thID]
        results = {}
        results["list"] = ["name", "lastname", "threadid"]
        query = self.create_entity(
            "FindEntity",
            class_str="AwesomePeople",
            constraints=constraints,
            results=results,
        )

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
        connections_arr = []

        for i in range(0, concurrency):
            db = self.create_connection()
            thread_add = Thread(target=self.addSingleEntity, args=(i, results, db))
            thread_add.start()
            thread_arr.append(thread_add)
            connections_arr.append(db)

        idx = 0
        error_counter = 0
        for th in thread_arr:
            th.join()
            if results[idx] == -1:
                error_counter += 1
            idx += 1

        self.assertEqual(error_counter, 0)

        for i in range(0, len(connections_arr)):
            self.disconnect(connections_arr[i])

        thread_arr = []
        connections_arr = []

        # Tests concurrent AddEntities and FindEntities (that should exists)
        results = [None] * concurrency * 2
        for i in range(0, concurrency):
            db1 = self.create_connection()
            addidx = concurrency + i
            thread_add = Thread(
                target=self.addSingleEntity, args=(addidx, results, db1)
            )
            thread_add.start()
            thread_arr.append(thread_add)
            connections_arr.append(db1)

            db2 = self.create_connection()
            thread_find = Thread(target=self.findEntity, args=(i, results, db2))
            thread_find.start()
            thread_arr.append(thread_find)
            connections_arr.append(db2)

        idx = 0
        error_counter = 0
        for th in thread_arr:
            th.join()
            if results[idx] == -1:
                error_counter += 1

            idx += 1

        self.assertEqual(error_counter, 0)

        for i in range(0, len(connections_arr)):
            self.disconnect(connections_arr[i])

    def test_addFindEntity(self):
        results = [None] * 1
        db = self.create_connection()
        self.addSingleEntity(0, results, db)
        self.findEntity(0, results, db)
        db.disconnect()

    def test_addEntityWithLink(self):
        db = self.create_connection()

        all_queries = []

        props = {}
        props["name"] = "Luis"
        props["lastname"] = "Ferro"
        props["age"] = 27
        query = self.create_entity(
            "AddEntity", ref=32, class_str="AwesomePeople", props=props
        )

        all_queries.append(query)

        props = {}
        props["name"] = "Luis"
        props["lastname"] = "Bueno"
        props["age"] = 27
        link = {}
        link["ref"] = 32
        link["direction"] = "in"
        link["class"] = "Friends"
        query = self.create_entity(
            "AddEntity", class_str="AwesomePeople", props=props, link=link
        )

        all_queries.append(query)

        response, res_arr = db.query(all_queries)

        self.assertEqual(response[0]["AddEntity"]["status"], 0)
        self.assertEqual(response[1]["AddEntity"]["status"], 0)
        db.disconnect()

    def test_addfindEntityWrongConstraints(self):
        db = self.create_connection()

        all_queries = []

        props = {"name": "Luis", "lastname": "Ferro", "age": 25}
        query = self.create_entity(
            "AddEntity", class_str="SomePeople", props=props, ref=32
        )

        all_queries.append(query)

        response, res_arr = db.query(all_queries)

        self.assertEqual(response[0]["AddEntity"]["status"], 0)

        all_queries = []

        # this format is invalid, as each constraint must be an array
        constraints = {"name": "Luis"}
        query = self.create_entity(
            "FindEntity",
            class_str="SomePeople",
            constraints=constraints,
            results={"count": ""},
        )

        all_queries.append(query)

        response, blob_arr = db.query(all_queries)

        self.assertEqual(response[0]["status"], -1)
        self.assertEqual(
            response[0]["info"], "Constraint for property 'name' must be an array"
        )

        # Another invalid format
        constraints = {"name": []}
        query = self.create_entity(
            "FindEntity",
            class_str="SomePeople",
            constraints=constraints,
            results={"count": ""},
        )
        all_queries = []
        all_queries.append(query)

        response, blob_arr = db.query(all_queries)

        self.assertEqual(response[0]["status"], -1)
        self.assertEqual(
            response[0]["info"],
            "Constraint for property 'name' must be an array of size 2 or 4",
        )
        db.disconnect()

    def test_FindWithSortKey(self):
        db = self.create_connection()

        all_queries = []

        number_of_inserts = 10

        for i in range(0, number_of_inserts):
            props = {}
            props["name"] = "entity_" + str(i)
            props["id"] = i

            query = self.create_entity(
                "AddEntity",
                class_str="Random",
                props=props,
            )

            all_queries.append(query)

        response, blob_arr = db.query(all_queries)

        self.assertEqual(len(response), number_of_inserts)
        for i in range(0, number_of_inserts):
            self.assertEqual(response[i]["AddEntity"]["status"], 0)

        all_queries = []

        results = {}
        results["list"] = ["name", "id"]
        results["sort"] = "id"

        query = self.create_entity("FindEntity", class_str="Random", results=results)
        all_queries.append(query)

        response, blob_arr = db.query(all_queries)

        self.assertEqual(response[0]["FindEntity"]["status"], 0)
        for i in range(0, number_of_inserts):
            self.assertEqual(response[0]["FindEntity"]["entities"][i]["id"], i)
        db.disconnect()

    def test_FindWithSortBlock(self):
        db = self.create_connection()

        all_queries = []

        number_of_inserts = 10

        for i in range(0, number_of_inserts):
            props = {}
            props["name"] = "entity_" + str(i)
            props["id"] = i

            query = self.create_entity(
                "AddEntity",
                class_str="SortBlock",
                props=props,
            )
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

        query = self.create_entity("FindEntity", class_str="SortBlock", results=results)
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

        query = self.create_entity("FindEntity", class_str="SortBlock", results=results)
        all_queries.append(query)

        response, blob_arr = db.query(all_queries)

        self.assertEqual(response[0]["FindEntity"]["status"], 0)
        for i in range(0, number_of_inserts):
            self.assertEqual(
                response[0]["FindEntity"]["entities"][i]["id"],
                number_of_inserts - 1 - i,
            )
        db.disconnect()

    def test_addEntityWithBlob(self, thID=0):
        db = self.create_connection()

        props = {}
        props["name"] = "Luis"
        props["lastname"] = "Ferro"
        props["age"] = 27
        props["threadid"] = thID

        query = self.create_entity(
            "AddEntity", class_str="AwesomePeople", props=props, blob=True
        )
        all_queries = []
        all_queries.append(query)

        blob_arr = []
        fd = open("../test_images/brain.png", "rb")
        blob_arr.append(fd.read())
        fd.close()

        response, res_arr = db.query(all_queries, [blob_arr])

        self.assertEqual(response[0]["AddEntity"]["status"], 0)
        self.disconnect(db)

    def test_addEntityWithBlobNoBlob(self, thID=0):
        db = self.create_connection()

        props = {}
        props["name"] = "Luis"
        props["lastname"] = "Ferro"
        props["age"] = 27
        props["threadid"] = thID
        query = self.create_entity(
            "AddEntity", class_str="AwesomePeople", props=props, blob=True
        )

        all_queries = []
        all_queries.append(query)

        response, res_arr = db.query(all_queries)

        self.assertEqual(response[0]["status"], -1)
        self.assertEqual(response[0]["info"], "Expected blobs: 1. Received blobs: 0")
        self.disconnect(db)

    def test_addEntityWithBlobAndFind(self, thID=0):
        db = self.create_connection()

        props = {}
        props["name"] = "Tom"
        props["lastname"] = "Slash"
        props["age"] = 27
        props["id"] = 45334

        query = self.create_entity(
            "AddEntity", class_str="NotSoAwesome", props=props, blob=True
        )
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

        query = self.create_entity(
            "FindEntity",
            class_str="NotSoAwesome",
            constraints=constraints,
            results=results,
        )
        all_queries = []
        all_queries.append(query)

        response, res_arr = db.query(all_queries)

        self.assertEqual(response[0]["FindEntity"]["entities"][0]["blob"], True)

        self.assertEqual(len(res_arr), len(blob_arr))
        self.assertEqual(len(res_arr[0]), len(blob_arr[0]))
        self.assertEqual((res_arr[0]), (blob_arr[0]))
        self.disconnect(db)
