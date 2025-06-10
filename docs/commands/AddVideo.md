# AddVideo Command

This call allows an application to add (and preprocess) a video in VDMS.
**The minimum requirement for this call is the video blob, a path to a video file on disk, or a mounted device**.
The video blob is an encoded binary array using any of the supported containers
or encodings.
While the application can choose
to associate no properties, it will make it
<!-- to associate no properties or no collections with this video, it will make it -->
harder to find the video later in such situations. In all scenarios, this call
will also lead to an addition of an entity into the
metadata database. If the information is provided to link this video with some
previously searched or added entity through a user/application specified id,
then such a link is also added to the metadata database.


## Parameters
* [optional] [`_ref`](../Keywords.md#keywords): for reference
* [optional] `codec`: Indicates the codec to be transcoded.
* [optional] `container`: Indicates the container used for the video file
* [optional] `from_file_path`: Add video using a file path on a local or mounted device.
* [optional] `is_local_file`: sSet this as true if a local copy of the file is required to be stored in VDMS data store.
* [optional] `index_frames`: Triggers key-frame index extraction on the video. This information will be used by [`FindFrames`](FindFrames.md) command to perform partial decoding rather than full decoding, which results in reduced memory consumption and faster command execution.


## Blocks
* [optional] [`properties`](../blocks/Block-properties.md)
* [optional] [`operations`](../blocks/Block-operations.md)
* [optional] [`link`](../blocks/Block-link.md)


## Codec and Containers

VDMS allows user to do transcoding at insertion and query time.
When inserting a new video (using the `AddVideo` command), the user can
choose to change the codec and/or container that will be used to store the
video in VDMS. If the user wants to keep the same encoder and containers
as passed in the video blob, the `codec` and `container` parameters must be skipped.


VDMS supports the following codecs: `xvid`, `h264`, and `h263`

VDMS supports the following video containers: `mp4`, `avi`, and `mov`


VDMS uses OpenCV/ffmpeg to operate on videos, so any codec/container available in those libraries can be easily included.


## Operations

VDMS allows for operations to be performed to a Video at insertion and/or
query time. The supported operations are detailed in the [operations](../blocks/Block-operations.md), and are [`threshold`, `resize`, `crop`, `interval`].
These operations were chosen as samples or were needed for some of the applications we were building. Given that we wrap around OpenCV for performing operations to videos, any operation supported by OpenCV can be
easily added.

At least one operation must be defined in the array.

All the parameters are required for each operation.


### Examples

Insert a video using mp4 as container for storage, and apply
a resize and threshold operation:
```json
{
    "AddVideo": {
        "container" : "mp4",  // will convert the video to mp4
        "properties" : {
            "name" : "The God Father",
            "length" : 120
        },
        "operations": [
            {
                "type": "resize",
                "w": 200,
                "h": 200,
            },
            {
                "type": "threshold",
                "value": 155,
            }
        ]
    }
}
+blob // the actual encoded video file as a blob
```

Insert a video after applying an `interval` operation.
The interval operation can be used to store only a part of the video (using
the start and end parameters), and/or to reduced change the number of frames per second (using the step parameters).
```json
{
    "AddVideo": {
        "properties" : {
            "name" : "Mean Girls",
            "length" : 134,
            "rating": "top"
        },
        "operations": [
            {
                "type": "interval", // Interval Operation
                "start": 20,        // Start from the 20th frame
                "stop": 200,        // Stop at the 200th frame
                "step": 5           // Pick 1 every 5 frames.
            }
        ]
    }
}
+blob // the actual encoded video file as a blob
```

Insert a video using a file path. Video object points to the file location and uses the file for all operations. When using an operation, the file at the given location will be modified.
Note that no blob should be sent with this query option.
```json
{
    "AddVideo": {
        "from_file_path": "/path/to/kinetics_test.mp4",
        "properties" : {
            "name" : "activity_test",
            "category" : "video_path_rop"
        },
        "operations" : [
            {
                "type": "resize",
                "width": 300,
                "height": 200
            },
        ]
    }
}
```

Insert a video using a file path. A duplicate copy is created and stored in the VDMS data store. The file at the given location is not modified or used subsequently.
Note that no blob should be sent with this query option.
```json
{
    "AddVideo": {
        "from_file_path": "path/to/kinetics_test.mp4",
        "is_local_file": True,
        "properties" : {
            "name" : "activity_test",
            "category" : "video_path_rop"
        },
        "operations" : [
            {
                "type": "resize",
                "width": 300,
                "height": 200
            },
        ]
    }
}
```
