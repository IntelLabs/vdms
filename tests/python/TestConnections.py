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

    def test_FindEntity_link_constraints_float(self):

        db = self.create_connection()

        props = {}
        props["name"] = "Jon"
        props["lastname"] = "Bonachon"
        props["age"] = 29

        response, arr = self.addEntity("felcflo_People", properties=props,
                                       check_status=True)

        props = {}
        props["type"] = "foo"
        props["name"] = "alligator"

        response, arr = self.addEntity("felcflo_foo", properties=props,
                                       check_status=True)

        props = {}
        props["type"] = "foo"
        props["name"] = "cat"

        response, arr = self.addEntity("felcflo_foo", properties=props,
                                       check_status=True)

        all_queries = []

        fE = {
            "FindEntity": {
                "class": "felcflo_People",
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
                "class": "felcflo_foo",
                "constraints": {
                    "name": ["==", "alligator"]
                },
                "_ref": 3
            }
        }
        all_queries.append(fE)

        fE = {
            "FindEntity": {
                "class": "felcflo_foo",
                "constraints": {
                    "name": ["==", "cat"]
                },
                "_ref": 4
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
                    "probablity": 0.3
                }
            }
        }
        all_queries.append(aC)

        aC = {

            "AddConnection": {
                "class": "foo_connection",
                "ref1": 2,
                "ref2": 4,
                "properties":{
                    "name": "best_type_of_connection",
                    "probablity": 0.6
                }
            }
        }
        all_queries.append(aC)

        response, res_arr = db.query(all_queries)

        self.assertEqual(response[0]["FindEntity"]["status"], 0)
        self.assertEqual(response[1]["FindEntity"]["status"], 0)
        self.assertEqual(response[2]["FindEntity"]["status"], 0)
        self.assertEqual(response[3]["AddConnection"]["status"], 0)
        self.assertEqual(response[4]["AddConnection"]["status"], 0)

        all_queries = []

        fE = {
            "FindEntity": {
                "class": "felcflo_People",
                "_ref": 1,
                "results": {
                    "list": ["name", "lastname"]
                }
            }
        }
        all_queries.append(fE)

        fE = {
            "FindEntity": {
                "class": "felcflo_foo",
                "link": {
                    "ref": 1,
                    "constraints": {
                        "probablity": [">=", 0.5],
                        "name": ["==", "best_type_of_connection"]
                    }

                },
                "results": {
                    "list": ["name"]
                }
            }
        }
        all_queries.append(fE)

        response, res_arr = db.query(all_queries)

        self.assertEqual(len(response[1]["FindEntity"]["entities"]), 1)
        self.assertEqual(response[1]["FindEntity"]["entities"][0]["name"], "cat")

        all_queries = []

        fE = {
            "FindEntity": {
                "class": "felcflo_People",
                "_ref": 1,
                "results": {
                    "list": ["name", "lastname"]
                }
            }
        }
        all_queries.append(fE)

        fE = {
            "FindEntity": {
                "class": "felcflo_foo",
                "link": {
                    "ref": 1,
                    "constraints": {
                        "probablity": [">=", 0.1],
                        "name": ["==", "best_type_of_connection"]
                    }

                },
                "results": {
                    "list": ["name"]
                }
            }
        }
        all_queries.append(fE)

        response, res_arr = db.query(all_queries)
        self.assertEqual(len(response[1]["FindEntity"]["entities"]), 2)

        all_queries = []

        fE = {
            "FindEntity": {
                "class": "felcflo_People",
                "_ref": 1,
                "results": {
                    "list": ["name", "lastname"]
                }
            }
        }
        all_queries.append(fE)

        fE = {
            "FindEntity": {
                "class": "felcflo_foo",
                "link": {
                    "ref": 1,
                    "constraints": {
                        "probablity": [">=", 1.0],
                        "name": ["==", "best_type_of_connection"]
                    }

                },
                "results": {
                    "list": ["name"]
                }
            }
        }
        all_queries.append(fE)

        response, res_arr = db.query(all_queries)
        self.assertEqual(len(response[1]["FindEntity"]["entities"]), 0)

    def test_FindEntity_link_constraints_string(self):

        db = self.create_connection()

        props = {}
        props["name"] = "Jon"
        props["lastname"] = "Bonachon"
        props["age"] = 29

        response, arr = self.addEntity("felcstr_People", properties=props,
                                       check_status=True)

        props = {}
        props["type"] = "foo"
        props["name"] = "alligator"

        response, arr = self.addEntity("felcstr_foo", properties=props,
                                       check_status=True)

        props = {}
        props["type"] = "foo"
        props["name"] = "cat"

        response, arr = self.addEntity("felcstr_foo", properties=props,
                                       check_status=True)

        all_queries = []

        fE = {
            "FindEntity": {
                "class": "felcstr_People",
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
                "class": "felcstr_foo",
                "constraints": {
                    "name": ["==", "alligator"]
                },
                "_ref": 3
            }
        }
        all_queries.append(fE)

        fE = {
            "FindEntity": {
                "class": "felcstr_foo",
                "constraints": {
                    "name": ["==", "cat"]
                },
                "_ref": 4
            }
        }
        all_queries.append(fE)

        aC = {

            "AddConnection": {
                "class": "foo_connection",
                "ref1": 2,
                "ref2": 3,
                "properties":{
                    "name": "best_type_of_connection_1",
                    "probablity": 0.3
                }
            }
        }
        all_queries.append(aC)

        aC = {

            "AddConnection": {
                "class": "foo_connection",
                "ref1": 2,
                "ref2": 4,
                "properties":{
                    "name": "best_type_of_connection",
                    "probablity": 0.6
                }
            }
        }
        all_queries.append(aC)

        response, res_arr = db.query(all_queries)

        self.assertEqual(response[0]["FindEntity"]["status"], 0)
        self.assertEqual(response[1]["FindEntity"]["status"], 0)
        self.assertEqual(response[2]["FindEntity"]["status"], 0)
        self.assertEqual(response[3]["AddConnection"]["status"], 0)
        self.assertEqual(response[4]["AddConnection"]["status"], 0)

        all_queries = []

        fE = {
            "FindEntity": {
                "class": "felcstr_People",
                "_ref": 1,
                "results": {
                    "list": ["name", "lastname"]
                }
            }
        }
        all_queries.append(fE)

        fE = {
            "FindEntity": {
                "class": "felcstr_foo",
                "link": {
                    "ref": 1,
                    "constraints": {
                        "name": ["==", "best_type_of_connection_1"]
                    }

                },
                "results": {
                    "list": ["name"]
                }
            }
        }
        all_queries.append(fE)

        response, res_arr = db.query(all_queries)

        self.assertEqual(len(response[1]["FindEntity"]["entities"]), 1)
        self.assertEqual(response[1]["FindEntity"]["entities"][0]["name"], "alligator")

        all_queries = []

        fE = {
            "FindEntity": {
                "class": "felcstr_People",
                "_ref": 1,
                "results": {
                    "list": ["name", "lastname"]
                }
            }
        }
        all_queries.append(fE)

        fE = {
            "FindEntity": {
                "class": "felcstr_foo",
                "link": {
                    "ref": 1,
                    "constraints": {
                        "name": [">=", "best_type_of_connection"]
                    }
                },
                "results": {
                    "list": ["name"]
                }
            }
        }
        all_queries.append(fE)

        response, res_arr = db.query(all_queries)
        self.assertEqual(len(response[1]["FindEntity"]["entities"]), 2)

        all_queries = []

        fE = {
            "FindEntity": {
                "class": "felcstr_People",
                "_ref": 1,
                "results": {
                    "list": ["name", "lastname"]
                }
            }
        }
        all_queries.append(fE)

        fE = {
            "FindEntity": {
                "class": "felcstr_foo",
                "link": {
                    "ref": 1,
                    "constraints": {
                        "name": ["<", "best_type_of_connection"]
                    }
                },
                "results": {
                    "list": ["name"]
                }
            }
        }
        all_queries.append(fE)

        response, res_arr = db.query(all_queries)
        self.assertEqual(len(response[1]["FindEntity"]["entities"]), 0)

        all_queries = []

        fE = {
            "FindEntity": {
                "class": "felcstr_People",
                "_ref": 1,
                "results": {
                    "list": ["name", "lastname"]
                }
            }
        }
        all_queries.append(fE)

        fE = {
            "FindEntity": {
                "class": "felcstr_foo",
                "link": {
                    "ref": 1,
                    "constraints": {
                        "name": ["==", "best_type_of_connection"]
                    }

                },
                "results": {
                    "list": ["name"]
                }
            }
        }
        all_queries.append(fE)

        response, res_arr = db.query(all_queries)
        self.assertEqual(len(response[1]["FindEntity"]["entities"]), 1)
        self.assertEqual(response[1]["FindEntity"]["entities"][0]["name"], "cat")
