# Image Tutorial

Continuing the [Simple Tutorial](Simple-Tutorial.md), we will add some images to
VDMS using the [AddImage](../commands/AddDescriptorSet.md) command, and we will link
those images to the "Hike" event and to the people that are part of that
photo.

We will use the Python Client module to connect to VDMS and send queries.

Let's start by connectin to VDMS.

```python
import vdms

db = vdms.VDMS()
db.connect("localhost") # Will connect to localhost on port 55555
```

First, lets read the images from some pre-defined location, and place.

```python
blob_arr = []

fd = open("hike/hiking_1.jpg")
blob_arr.append(fd.read())
```

Let's assume that our friend "Sophia" is present in the image.
Let's insert the image and represent the fact that Sophia is in that image.
For this, we need a reference to the entity
representing "Sophia". So, let's use [FindEntity](../commands/FindEntity.md) to find "Sophia",
and use a reference to link "Sophia" to the new image.

```python

all_queries = []

query = {
          "FindEntity" : {
             "class": "Person",
             "_ref": 3,                     # We set the reference as 3
             "constraints": {
                 "name": ["==", "Sophia"]
             }
          }
       }

all_queries.append(query)
```

And now, let's include the image (using the [AddImage](../commands/AddImage.md))
and connect it to Sophia (using the [link](../blocks/Block-link.md) block).
Now, assume we want all the images to be stored in a certain size, which
may not be the same size as we read the image from disk.
We can use the [operations](../blocks/Block-operations.md) block for this. The operation
will be applied at insertion time.

```python
props = {}
props["name_file"] = "hiking_1"
props["type"] = "hike"
props["date"] = "March 24th, 2017"

resize = {}
resize["type"] = "resize"
resize["height"] = 500
resize["width"]  = 400

operations = []
operations.append(resize)

link = {}
link["ref"] = 3                 # We use the set reference
link["direction"] = "in"
link["class"] = "is_in"

addImage = {}
addImage["properties"] = props
addImage["operations"] = operations
addImage["link"] = link
addImage["format"] = "jpg"      # Format use to store it on VDMS

query = {}
query["AddImage"] = addImage

all_queries.append(query)

print "Query Sent:"
vdms.aux_print_json(all_queries)

response, res_arr = db.query(all_queries, [blob_arr])
```

We should see:

        Query Sent:
        [
            {
              "FindEntity" : {
                 "class": "Person",
                 "_ref": 3,
                 "constraints": {
                     "name": ["==", "Sophia"]
                 }
              }
            },
            {
                "AddImage": {
                    "link": {
                        "ref": 3
                    },
                    "operations": [
                        {
                            "type": "resize",
                            "height": 400,
                            "width": 500
                        }
                    ],
                    "properties": {
                        "name_file": "hiking_1",
                        "type": "hike",
                        "date": "March 24th, 2017"
                    }
                }
            }
        ]


**Note** that we are sending the image using the blob field in:

    db.query(all_queries, **[blob_arr]**)

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
                    "status": 0,
                    "returned": 1
                }
            },
            {
                "AddImage": {
                    "status": 0
                }
            }
        ]

Awesome! we have our first image and its metadata pushed to VDMS.
