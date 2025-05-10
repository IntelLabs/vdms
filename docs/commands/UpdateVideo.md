### UpdateVideo Command

Updates existing video(s). This command provides ways to find one or more
existing video and then lets an application add, change or remove properties.
The local id allows an application to chain this command with following operations
or associate with a previous search.

### Parameters

* [optional] _ref: for reference.

### Blocks
* [optional] properties
* [optional] remove_props
* [optional] constraints

** At least one of _ref or class + constraints are required for searching. **
If _ref is provided, it essentially refers to the search results of a previous
FindVideo or AddVideo. Since the Update call just results in updating the
property values inline within those entities, we keep the same _ref value. The
call can result in an update to multiple entities depending on the search
criteria. Unique is not assumed to be true in this case.

Property keys are case sensitive and are matched exactly when updating properties.

If search results are empty, this will cause an exception.


### Examples

    // Add the video entity representing `video_1`.

    "AddVideo" : {
        "_ref": 1234
        "properties" : {
            "Name" : "video_1",
            "fps" : 30
        }
    }

    // Remove the fps property, assuming the "Name" property
    // uniquely identifies a video.

    "UpdateVideo" : {
        "_ref": 1234
        "constraints" : {
            "Name": [ "==", "video_1" ],
        }
        "remove_props" : [ "fps" ]
    }
