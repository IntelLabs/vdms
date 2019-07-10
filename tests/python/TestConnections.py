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

class TestConnections(TestCommand.TestCommand):

    def test_FindConnection_LinkedTo_Entity(self):

        db = self.create_connection()

        props = {}
        props["name"] = "Jon"
        props["lastname"] = "Bonachon"
        props["age"] = 29

        response, arr = self.addEntity("ConnectionPeople", properties=props,
                                       check_status=True)

        props = {}
        props["type"] = "foo"
        props["name"] = "alligator"

        response, arr = self.addEntity("foo", properties=props,
                                       check_status=True)

        all_queries = []

        fE = {
            "FindEntity": {
                "class": "ConnectionPeople",
                "constraints": {
                    "name": ["==", "Jon"],
                    "lastname": ["==", "Bonachon"],
                },
                "_ref": 2
            }
        }
        all_queries.append(fE)

        fE = {
            "FindEntity": {
                "class": "foo",
                "constraints": {
                    "name": ["==", "alligator"]
                },
                "_ref": 3
            }
        }
        all_queries.append(fE)

        aC = {

            "AddConnection": {
                "class": "foo_connection",
                "ref1": 2,
                "ref2": 3,
                "properties":{
                    "name": "best_type_of_connection",
                    "val": 0.32
                }
            }
        }
        all_queries.append(aC)

        response, res_arr = db.query(all_queries)

        self.assertEqual(response[0]["FindEntity"]["status"], 0)
        self.assertEqual(response[1]["FindEntity"]["status"], 0)
        self.assertEqual(response[2]["AddConnection"]["status"], 0)

        all_queries = []

        fE = {
            "FindEntity": {
                "class": "ConnectionPeople",
                "constraints": {
                    "name": ["==", "Jon"],
                    "lastname": ["==", "Bonachon"],
                },
                "_ref": 2
            }
        }
        all_queries.append(fE)

        fC = {
            "FindConnection": {
                "_ref": 30,
                "ref1": 2,
                "constraints": {
                    "val": [">=", 0.3]
                },
                "results": {
                    "list": ["name", "val"]
                }
            }
        }
        all_queries.append(fC)

        fE = {
            "FindEntity": {
                "class": "foo",
                "link": {
                    "ref": 30
                },
                "results": {
                    "list": ["name"]
                }
            }
        }
        all_queries.append(fE)

        import json
        print(json.dumps(all_queries, indent=4, sort_keys=False))

        response, res_arr = db.query(all_queries)

        print(db.get_last_response_str())
