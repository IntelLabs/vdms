# FindFrames Command

Works similar to [[FindVideo|FindVideo]], except that the return type is a list of frames within the video rather than the video blob itself. All operations that can be performed on images are also applied for this command.

The list of images to be returned is defined in two ways: either by specifying the _frames_ parameter, or by passing _interval_ as part of _operations_ block. The command fails in presence of both parameters, or in absence of either one of them.

Note that the interval parameter is not supported by commands that operate on images. This is an exception for FindFrames command, as the interval parameter is used as means to specify the list of frames.

## Parameters

* frames: list of frame indexes
* [optional] _ref: for reference
* [optional] format: Specify the format used for returned images [jpg, png]. Default is png.
* [optional] unique: [True or False]. Indicates whether a single element is expected to match that condition.

## Blocks

* [optional] [[operations|Block-operations]]: in addition to image operations, an interval can be specified (if frames parameter is not given already)
* [optional] [[results|Block-results]]
* [optional] [[constraints|Block-constraints]]
* [optional] [[link|Block-link]]

## Examples
    // Find the video and returns 1st, 10th, and 100th frames of it
    {
      "FindFrames": {
          "frames" : [0, 9, 99],
          "constraints" : {
              "name" : ["==", "Spongebob Squarepants"},
          }
      }
    }

    // Same as above, except for using the interval operation
    {
      "FindFrames": {
          "constraints" : {
              "name" : ["==", "Family Guy"},
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
