# AddBoundingBox Command

Bounding Boxes are a special kind of entity, similar to images.
The `AddBoundingBox` call allows applications to add a region of interest
to VDMS, where the entity added has a preset class of BoundingBox.

## Parameters
* [optional] [`_ref`](../Keywords.md#keywords): for reference
* [optional] `image`: Reference to an existing image/frame (from an AddImage or FindImage call earlier in the transaction)

## Blocks
* [required] [`rectangle`](../blocks/Block-rectangle.md)
* [optional] [`properties`](../blocks/Block-properties.md)
<!-- * [optional] [`collections`](../blocks/Block-collections.md) -->


The `image` parameter allows VDMS to create a connection between the image or frame
and the BoundingBox. While it is not required, it is highly recommended.

In addition, there must be a `rectangle` block (indicating a rectangular region of
interest) in order to add a BoundingBox to VDMS.


## Examples

Add a rectangular region of interest:
```JSON
"AddImage" : {
    "_ref" : 1234,
    "format" : "png",

    "properties" : {
        "name" : "interesting_stuff"
    }
}
"AddBoundingBox" : {
    "_ref" : 1,
    "image": 1234,
    "rectangle" : {"x": 120, "y": 50, "w": 40, "h": 40}
}
```

Add a region of interest and specify properties:
```JSON
"AddImage" : {
    "_ref" : 1234,
    "format" : "png",

    "properties" : {
        "name" : "interesting_stuff"
    }
}
"AddBoundingBox" : {
    "_ref" : 1235,
    "image" : 1234,
    "rectangle" : {"x": 120, "y": 50, "w": 40, "h": 60},
    "properties" : {
        "name" : "Jane Doe"
    }
}
```
