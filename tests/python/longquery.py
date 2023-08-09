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

import os


def queryPerson(id):
    query = [
        {
            "AddEntity": {
                "_ref": 1,
                "class": "Person",
                "properties": {"Id": id, "imaginary_node": 1},
            }
        },
        {
            "AddEntity": {
                "_ref": 2,
                "class": "BoundingBox",
                "properties": {
                    "Height": "267",
                    "Id": id,
                    "Width": "117",
                    "X": "296",
                    "Y": "496",
                },
            }
        },
        {
            "AddDescriptor": {
                "_ref": 3,
                "label": "Person",
                "properties": {
                    "id": id,
                    "tag": "person",
                    "time_stamp": {"_date": "Sat Jan 06 23:00:00 PST 83186920"},
                },
                "set": "features_vectors_store1",
            }
        },
        {"AddConnection": {"class": "Has", "ref1": 1, "ref2": 3}},
        {"AddConnection": {"class": "Represents", "ref1": 1, "ref2": 2}},
        {"AddConnection": {"class": "AppearsIn", "ref1": 3, "ref2": 2}},
    ]

    return query


def queryVisit(id):
    query = [
        {
            "AddEntity": {
                "_ref": 4,
                "class": "Visit",
                "constraints": {"Id": ["==", id]},
                "properties": {
                    "Id": id,
                    "ending_time": {"_date": "Sat Jan 06 23:03:00 PDT 2018"},
                    "starting_time": {"_date": "Sat Jan 06 23:00:00 PST 83186920"},
                },
            }
        },
        {
            "FindEntity": {
                "_ref": 5,
                "class": "Person",
                "constraints": {"Id": ["==", id]},
            }
        },
        {"AddConnection": {"class": "visited", "ref1": 4, "ref2": 5}},
        {
            "FindEntity": {
                "_ref": 6,
                "class": "Store",
                "constraints": {"Name": ["==", "Walmart"]},
            }
        },
        {"AddConnection": {"class": "Store_Visit", "ref1": 4, "ref2": 6}},
        {
            "FindEntity": {
                "_ref": 7,
                "class": "Area",
                "constraints": {"Name": ["==", "Area15"]},
            }
        },
        {
            "AddConnection": {
                "class": "PassBy",
                "properties": {
                    "Area": "Area15",
                    "ending_time": {"_date": "Sat Jan 06 23:03:00 PDT 2018"},
                    "passing_time": {"_date": "Sat Jan 06 23:00:00 PST 83186920"},
                },
                "ref1": 4,
                "ref2": 7,
            }
        },
        {
            "FindEntity": {
                "_ref": 8,
                "class": "Area",
                "constraints": {"Name": ["==", "Area14"]},
            }
        },
        {
            "AddConnection": {
                "class": "PassBy",
                "properties": {
                    "Area": "Area14",
                    "ending_time": {"_date": "Sat Jan 06 23:03:00 PDT 2018"},
                    "passing_time": {"_date": "Sat Jan 06 23:00:00 PST 83186920"},
                },
                "ref1": 4,
                "ref2": 8,
            }
        },
        {
            "FindEntity": {
                "_ref": 9,
                "class": "Area",
                "constraints": {"Name": ["==", "Area13"]},
            }
        },
        {
            "AddConnection": {
                "class": "PassBy",
                "properties": {
                    "Area": "Area13",
                    "ending_time": {"_date": "Sat Jan 06 23:03:00 PDT 2018"},
                    "passing_time": {"_date": "Sat Jan 06 23:00:00 PST 83186920"},
                },
                "ref1": 4,
                "ref2": 9,
            }
        },
        {
            "FindEntity": {
                "_ref": 10,
                "class": "Area",
                "constraints": {"Name": ["==", "Area12"]},
            }
        },
        {
            "AddConnection": {
                "class": "PassBy",
                "properties": {
                    "Area": "Area12",
                    "ending_time": {"_date": "Sat Jan 06 23:03:00 PDT 2018"},
                    "passing_time": {"_date": "Sat Jan 06 23:00:00 PST 83186920"},
                },
                "ref1": 4,
                "ref2": 10,
            }
        },
        {
            "FindEntity": {
                "_ref": 11,
                "class": "Area",
                "constraints": {"Name": ["==", "Area11"]},
            }
        },
        {
            "AddConnection": {
                "class": "PassBy",
                "properties": {
                    "Area": "Area11",
                    "ending_time": {"_date": "Sat Jan 06 23:03:00 PDT 2018"},
                    "passing_time": {"_date": "Sat Jan 06 23:00:00 PST 83186920"},
                },
                "ref1": 4,
                "ref2": 11,
            }
        },
        {
            "FindEntity": {
                "_ref": 12,
                "class": "Area",
                "constraints": {"Name": ["==", "Area10"]},
            }
        },
        {
            "AddConnection": {
                "class": "PassBy",
                "properties": {
                    "Area": "Area10",
                    "ending_time": {"_date": "Sat Jan 06 23:03:00 PDT 2018"},
                    "passing_time": {"_date": "Sat Jan 06 23:00:00 PST 83186920"},
                },
                "ref1": 4,
                "ref2": 12,
            }
        },
        {
            "FindEntity": {
                "_ref": 13,
                "class": "Area",
                "constraints": {"Name": ["==", "Area9"]},
            }
        },
        {
            "AddConnection": {
                "class": "PassBy",
                "properties": {
                    "Area": "Area9",
                    "ending_time": {"_date": "Sat Jan 06 23:03:00 PDT 2018"},
                    "passing_time": {"_date": "Sat Jan 06 23:00:00 PST 83186920"},
                },
                "ref1": 4,
                "ref2": 13,
            }
        },
        {
            "FindEntity": {
                "_ref": 14,
                "class": "Area",
                "constraints": {"Name": ["==", "Area8"]},
            }
        },
        {
            "AddConnection": {
                "class": "PassBy",
                "properties": {
                    "Area": "Area8",
                    "ending_time": {"_date": "Sat Jan 06 23:03:00 PDT 2018"},
                    "passing_time": {"_date": "Sat Jan 06 23:00:00 PST 83186920"},
                },
                "ref1": 4,
                "ref2": 14,
            }
        },
        {
            "FindEntity": {
                "_ref": 15,
                "class": "Area",
                "constraints": {"Name": ["==", "Area7"]},
            }
        },
        {
            "AddConnection": {
                "class": "PassBy",
                "properties": {
                    "Area": "Area7",
                    "ending_time": {"_date": "Sat Jan 06 23:03:00 PDT 2018"},
                    "passing_time": {"_date": "Sat Jan 06 23:00:00 PST 83186920"},
                },
                "ref1": 4,
                "ref2": 15,
            }
        },
        {
            "FindEntity": {
                "_ref": 16,
                "class": "Area",
                "constraints": {"Name": ["==", "Area6"]},
            }
        },
        {
            "AddConnection": {
                "class": "PassBy",
                "properties": {
                    "Area": "Area6",
                    "ending_time": {"_date": "Sat Jan 06 23:03:00 PDT 2018"},
                    "passing_time": {"_date": "Sat Jan 06 23:00:00 PST 83186920"},
                },
                "ref1": 4,
                "ref2": 16,
            }
        },
        {
            "FindEntity": {
                "_ref": 17,
                "class": "Area",
                "constraints": {"Name": ["==", "Area5"]},
            }
        },
        {
            "AddConnection": {
                "class": "PassBy",
                "properties": {
                    "Area": "Area5",
                    "ending_time": {"_date": "Sat Jan 06 23:03:00 PDT 2018"},
                    "passing_time": {"_date": "Sat Jan 06 23:00:00 PST 83186920"},
                },
                "ref1": 4,
                "ref2": 17,
            }
        },
        {
            "FindEntity": {
                "_ref": 18,
                "class": "Area",
                "constraints": {"Name": ["==", "Area4"]},
            }
        },
        {
            "AddConnection": {
                "class": "PassBy",
                "properties": {
                    "Area": "Area4",
                    "ending_time": {"_date": "Sat Jan 06 23:03:00 PDT 2018"},
                    "passing_time": {"_date": "Sat Jan 06 23:00:00 PST 83186920"},
                },
                "ref1": 4,
                "ref2": 18,
            }
        },
        {
            "FindEntity": {
                "_ref": 19,
                "class": "Area",
                "constraints": {"Name": ["==", "Area3"]},
            }
        },
        {
            "AddConnection": {
                "class": "PassBy",
                "properties": {
                    "Area": "Area3",
                    "ending_time": {"_date": "Sat Jan 06 23:03:00 PDT 2018"},
                    "passing_time": {"_date": "Sat Jan 06 23:00:00 PST 83186920"},
                },
                "ref1": 4,
                "ref2": 19,
            }
        },
        {
            "FindEntity": {
                "_ref": 20,
                "class": "Area",
                "constraints": {"Name": ["==", "Area2"]},
            }
        },
        {
            "AddConnection": {
                "class": "PassBy",
                "properties": {
                    "Area": "Area2",
                    "ending_time": {"_date": "Sat Jan 06 23:03:00 PDT 2018"},
                    "passing_time": {"_date": "Sat Jan 06 23:00:00 PST 83186920"},
                },
                "ref1": 4,
                "ref2": 20,
            }
        },
        {
            "FindEntity": {
                "_ref": 21,
                "class": "Area",
                "constraints": {"Name": ["==", "Area1"]},
            }
        },
        {
            "AddConnection": {
                "class": "PassBy",
                "properties": {
                    "Area": "Area1",
                    "ending_time": {"_date": "Sat Jan 06 23:03:00 PDT 2018"},
                    "passing_time": {"_date": "Sat Jan 06 23:00:00 PST 83186920"},
                },
                "ref1": 4,
                "ref2": 21,
            }
        },
    ]

    return query
