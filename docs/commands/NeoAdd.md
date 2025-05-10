# NeoAdd Command

NeoAdd is one of two supported commands for the experimental Neo4J based scale-out VDMS.

In spirit, it combines the various "Add" commands (AddEntity, AddImage, etc) from PMGD based VDMS into a unified command for adding/updating metadata and data to VDMS.


Parameters:
* target_data_type: currently this supports images ("img") and metadata ("md_only"). If an image is specified, an associated blob must be uploaded with it.
* [optional]target_format: specifies the targeted file format for uploaded images, e.g. jpg
* cypher: this is where specially formatted cypher is used to specify the metadata to be either uploaded or retrieved. See below for more details.

Blocks:
* [optional] operations

## Cypher

The cypher field, which exposes raw-cypher graph query language to the client. While this makes the call a bit more complicated, it removes the need for complex query daisy chaining using the _ref field in the existing API. Note this assumes a basic knowledge of the cypher GQL, [see tutorials here](https://neo4j.com/docs/getting-started/appendix/tutorials/guide-cypher-basics/), though you can copy-paste-modify our examples here to play around.


To ensure VDMS can properly store system metadata,  the primary query must have a variable named VDMSNODE that refers to the nodes being created or retrieved. This is so the VDMS server can append system metadata to the relevant graph nodes, such as data locations and type specific labels (note that labels are roughly equivalent to PMGD classes).

For example, to create a graph new node with the label “cat” and properties for fur type and color we would have a cypher query that looks like this:

`CREATE(VDMSNODE:Cat{color:"orange",fur_type:"long"});`

We can include arbitrary numbers of labels within the same node as well.

`CREATE(VDMSNODE:Cat:animal:mammal{color:"orange",fur_type:"long"});`

## Example Calls

### Adding an image with properties and operations

Pulling it all together, lets create a call using NeoAdd that uploads an image, crops it, and stores the metadata specified in cypher.
```
[
    {
     "NeoAdd":
     {
         “cypher”: “CREATE(VDMSNODE:Cat{color:\"orange\",fur_type:\"long\"})”
   "target_format": "png",
          “target_data_type”: “img”,

	  "operations": [
            {
            "type": "crop",
            "x": 0, "y": 0,
            "width": 640, "height": 480,
             },
	]

        }
    }
]
```

### Creating a new metadata node

And a pure metadata example (akin to AddEntity with no binary blobs)

And for metadata creation
```
[
    {
     "NeoAdd":
     {
            “cypher”: “CREATE(VDMSNODE:Cat{color:\"orange\",fur_type:\"long\"})”,
          “target_data_type”: “md_only”,

        }
    }
]
```
### Creating an image node, and entity node, and linking them together

As a slightly more sophisticated example, this will crop the uploaded image, as well as create two graph nodes and a relationship between them.
Note that the node that we want to affiliate the image with still has to have the VDMSNODE variable assigned to it.

Note that the node we are creating a link to also has properties, and the link itself may have labels and properties as well. In this case, just a single label of "LinksTo"

```
[
    {
     "NeoAdd":
     {
         “cypher”: “CREATE(VDMSNODE:Cat{color:\"orange\",fur_type:\"long\"}) -[:LinksTo] -> (:OtherNode {OtherProperty:\"Bar\"})”
   "target_format": "png",
          “target_data_type”: “img”,

	  "operations": [
            {
            "type": "crop",
            "x": 0, "y": 0,
            "width": 640, "height": 480,
             },
	]

        }
    }
]
```

