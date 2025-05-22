# Simple Tutorial

In this example, we will use VDMS to store all the photos and metadata
taken during a hiking day at Mount Rainier, WA.

We will use the Python Client module to connect to VDMS and send queries.

Let's start by connection to VDMS.

```python
import vdms
import json

db = vdms.vdms()
db.connect("localhost") # Will connecto to localhost on port 55555
```

Now, let's add some metadata about the place of the hiking.

```python
props = {}
props["place"] = "Mt Rainier"
props["id"] = 4543
props["type"] = "Volcano"

addEntity = {}
addEntity["properties"] = props
addEntity["class"] = "Hike"
```

Finally, lets create a query and send it to VDMS.

```python
query = {}
query["AddEntity"] = addEntity

all_queries = []
all_queries.append(query)

print "Query Sent: "
print(all_queries)

response, res_arr = db.query(all_queries)
```

This will print:

        Query Sent:
        [
            {
                "AddEntity": {
                    "properties": {
                        "type": "Volcano",
                        "place": "Mt Rainier",
                        "id": 4543
                    },
                    "class": "Hike"
                }
            }
        ]


Now, lets print the response we get from VDMS

```python
response = json.loads(response)

print "VDMS Response:"
vdms.aux_print_json(response)
```

This will print:

        VDMS Response:
        [
            {
                "AddEntity": {
                    "status": 0
                }
            }
        ]

Alright! This means that the server added that entity successfully. Now lets
add two people and connect them to this hike.
For this, we will use the [FindEntity](../commands/FindEntity.md) command (to get a reference
to the "Hike" entity we added), and the [link](../blocks/Block-link.md) block inside
the [AddEntity](../commands/AddEntity.md) (for adding a "Person") to define the connection.

```python
# As an example, we build the FindEntity command directly
query = {
          "FindEntity" : {
             "class": "Hike",
             "_ref": 3,
             "constraints": {
                 "id": ["==", 4543]
             }
          }
       }

all_queries = []
all_queries.append(query)

props = {}
props["name"]     = "Tom"
props["lastname"] = "Lluhs"
props["id"]       = 453

link = {}
link["ref"] = 3

addEntity = {}
addEntity["properties"] = props
addEntity["class"] = "Person"
addEntity["link"] = link

query = {}
query["AddEntity"] = addEntity

all_queries.append(query)

props = {}
props["name"]     = "Sophia"
props["lastname"] = "Ferdinand"
props["id"]       = 454

link = {}
link["ref"] = 3

addEntity = {}
addEntity["properties"] = props
addEntity["class"] = "Person"
addEntity["link"] = link

query = {}
query["AddEntity"] = addEntity

all_queries.append(query)

print "Query Sent:"
print(all_queries)

response, res_arr = db.query(all_queries)
```

The query sent will be:

        Query Sent:
        [
            {
                "FindEntity": {
                    "_ref": 3,
                    "class": "Hike",
                    "constraints": {
                        "id": [ "==", 4543 ]
                    }
                }
            },
            {
                "AddEntity": {
                    "link": {
                        "ref": 3
                    },
                    "properties": {
                        "lastname": "Tom",
                        "name": "Lluhs",
                        "id": 0
                    },
                    "class": "Person"
                }
            },
            {
                "AddEntity": {
                    "link": {
                        "ref": 3
                    },
                    "properties": {
                        "lastname": "Ferdinand",
                        "name": "Isof",
                        "id": 2
                    },
                    "class": "Person"
                }
            }
        ]

**Note** that nothing prevent us from inserting the "Hike" entity together
with the "Person" entities all together in a single transaction. We use
separate transactions just for demonstration purposes.


Now let's print the response we get from VDMS

```python
response = json.loads(response)

print "VDMS Response:"
vdms.aux_print_json(response)
```

        VDMS Response:
        [
            {
                "FindEntity": {
                    "status": 0
                }
            },
            {
                "AddEntity": {
                    "status": 0
                }
            },
            {
                "AddEntity": {
                    "status": 0
                }
            }
        ]

