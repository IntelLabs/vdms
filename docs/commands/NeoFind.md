### NeoFind Command

NeoFind is one of two supported commands for the experimental Neo4J based scale-out VDMS.

It combines the various "Find" commands from PMGD based VDMS into a single unified command for retrieval of metadata and data from VDMS.

Parameters:

* target_data_type: currently this supports images ("img") and metadata ("md_only"). If an image is specified, an associated blob will be downloaded.
* [optional]target_format: specifies the targeted file format for the data to be returned as, e.g. jpg
* cypher: this is where specially formatted cypher is used to specify the metadata to be retrieved. See here below for more details.

Blocks:
* [optional] operations

## Cypher

The cypher field, which exposes raw-cypher graph query language to the client. While this makes the call a bit more complicated, it removes the need for complex query daisy chaining using the _ref field in the existing API. Note this assumes a basic knowledge of the cypher GQL, [see tutorials here](https://neo4j.com/docs/getting-started/appendix/tutorials/guide-cypher-basics/), though you can copy-paste-modify our examples here to play around.

There are a couple rules that should be followed to ensure VDMS does what its supposed to for each call.

The primary one is that the primary query must have a variable named VDMSNODE that refers to the nodes being created or retrieved. This is so the VDMS server can append system metadata to the relevant graph nodes, such as data locations and type specific labels (note that labels are roughly equivalent to PMGD classes).

For example, to match nodes with the label “cat” and return the property fur type:

`MATCH (VDMSNODE:Cat) RETURN VDMSNODE.fur_type`

## Example Calls

### Find and return an image and property

Here, we are looking for all images that match the label “cat”, return their fur type, and crop the image
```
[
    {
        "NeoFind":
     {
            “cypher”: “MATCH (VDMSNODE:Cat) RETURN VDMSNODE.fur_type;”)”,
         "target_format": "jpg",
          “target_data_type”: “img”,
	  "operations": [
            {
            "type": "crop",
            "x": 0, "y": 0,
            "width": 256, "height": 256,
             },
	]

        }
    }
]
```
### Metadata only query

And similar for metadata only returns
```
[
    {
        "NeoFind":
     	{
            		“cypher”: “MATCH (VDMSNODE:Cat) RETURN VDMSNODE.fur_type;”)”,
          	“target_data_type”: “md_only”,
        }
    }
]
```

### Metadata Only + Link Traversal
Here we match all nodes that link to node(s) with the tag "cat" and return them.
```
[
    {
        "NeoFind":
     	{
            		“cypher”: “MATCH (tgt:Cat) -[]-(VDMSNODE) RETURN VDMSNODE)”,
          	“target_data_type”: “md_only”,
        }
    }
]
```


