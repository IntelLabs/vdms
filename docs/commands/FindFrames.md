# FindFrames Command

Works similar to [FindVideo](FindVideo.md), except that the return type is a list of frames within the video rather than the video blob itself.
All operations that can be performed on images are also applied for this command.

The list of images to be returned is defined in two ways: either by specifying the `frames` parameter, or by passing `interval` as part of `operations` block. The command fails in presence of both parameters, or in absence of either one of them.

***Note:*** The `interval` parameter is not supported by commands that operate on images. This is an exception for `FindFrames` command, as the interval parameter is used as means to specify the list of frames.


## Parameters

* `frames`: list of frame indexes
* [optional] [`_ref`](../Keywords.md#keywords): for reference
* [optional] `format`: Specify the format used for returned images [`jpg`, `png`]. Default is `png`.
* [optional] `unique`: [True or False]. Indicates whether a single element is expected to match that condition.


## Blocks

* [optional] [`operations`](../blocks/Block-operations.md): in addition to image operations, an interval can be specified (if frames parameter is not given already)
* [required] [results](../blocks/Block-results.md)
* [required] [`constraints`](../blocks/Block-constraints.md)
* [optional] [`link`](../blocks/Block-link.md)


## Examples
Find the video "Spongebob Squarepants" and returns 1st, 10th, and 100th frames.
```JSON
{
    "FindFrames": {
        "frames" : [0, 9, 99],
        "constraints" : {
            "name" : ["==", "Spongebob Squarepants"]
        }
    }
}
```

Find the video "Family Guy", return every 5th frame (using the interval operation) with pixel values above 155.
```JSON
{
    "FindFrames": {
        "constraints" : {
            "name" : ["==", "Family Guy"],
        },
        "operations": [
            {
                "type": "interval",
                "start": 0,
                "stop" : 99,
                "step" : 5
            },
            {
                "type": "threshold",
                "value": 155,
            }
        ]
    }
}
```
