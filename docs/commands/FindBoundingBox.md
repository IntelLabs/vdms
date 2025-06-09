# Finding Bounding Boxes

`FindBoundingBox` will not return anything by default. If the bounding box was associated with an image or frame,
the `blob` keyword can be set to true in the [results](../blocks/Block-results.md) block and the pixel data will be returned. Likewise, including the `_coordinates` keyword in the `list` option of the [results](../blocks/Block-results.md) parameter will return a list of the coordinates matching the specified criteria.


## Parameters
* [optional] [`_ref`](../Keywords.md#keywords): Local id to be used as reference for a given entity.
* [optional] `unique`: Indicates whether a single element is expected to match that condition.
* [optional] `image`: Reference to an image or a frame


## Blocks
* [required] [`rectangle`](../blocks/Block-rectangle.md)
* [required] [`constraints`](../blocks/Block-constraints.md)
<!-- * [required] [collections](../blocks/Block-collections.md) -->
* [required] [results](../blocks/Block-results.md)


If nothing is specified, all bounding boxes in VDMS are returned.
If `rectangle` is specified, the bounding boxes returned must be fully within the dimensions indicated by the given rectangle.
If `image` is specified, the bounding boxes must be from that image or frame.

Common usage of this call is to specify an image that contains bounding boxes (as in
 "find all the regions of interest in this image") or to find all bounding boxes
with a given name ("find all regions of interest that correspond to Jane").


## Examples

Find the regions of interest from the image named "interesting_stuff"
```JSON
"FindImage" : {
    "_ref" : 1,
    "constraints" : {
        "name" : [ "==", "interesting_stuff"]
    }
},
"FindBoundingBox": {
    "_ref" : 2,
    "image" : 1,
    "results": {
        "list": ["_coordinates"]
    }
}
```

Find all regions of interest that represent Jane Doe:
```JSON
"FindBoundingBox": {
    "_ref" : 2,
    "constraints" : {
        "name" : ["==", "Jane Doe"]
    },
    "results": {
        "blob": true
    }
}
```
