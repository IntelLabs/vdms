### Updating Bounding Boxes

The UpdateBoundingBox call allows applications to update a region of interest that exists in VDMS.

### Parameters
* [optional] [[_ref|Keywords]]: for reference

### Blocks
* [optional] [[rectangle|Block-rectangle]]
* [optional] [[properties|Block-properties]]
* [optional] [[remove_props|Block-properties]]
* [optional] [[constraints|Block-constraints]]

Either **_ref** or **constraints** must be given in order to determine what exactly to update. If _ref is provided, it refers to the search results of a previous [[FindBoundingBox|FindBoundingBox]] or [[AddBoundingBox|AddBoundingBox]]. Since the Update call just results in updating the property values inline within those entities, we keep the same _ref value.

If the **rectangle** block is included in the update call, the user can update the coordinates of the BoundingBox. USE THIS CAREFULLY: if you do not set **_ref**, choose your **constraints** well or you may change a bounding box that you did not intend to. The call can result in an update to multiple entities depending on the search criteria. Unique is not assumed to be true in this case.

If **rectangle** is absent, only properties will be updated (or removed).

### Examples

    // Update the name of a region of interest
    "UpdateBoundingBox" : {
        "_ref" : 1234,
       "properties" : {
            "name" : "super_interesting_stuff"
       }
    }

    // Update the coordinates of a region of interest
    "UpdateBoundingBox" : {
        "_ref" : 1234,
        "rectangle" : {"x": 100, "y": 60, "w": 45, "h": 45}
    }

    // Update the coordinates of a region of interest based on constraints
    "UpdateBoundingBox" : {
        "rectangle" : {"x": 100, "y": 100, "w": 35, "h": 35},
        "constraints" : { "name" : ["==", "super_interesting_stuff"] }
    }