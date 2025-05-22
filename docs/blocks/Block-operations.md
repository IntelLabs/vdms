# operations

VDMS allows for operations to be performed to an image and videos at insertion and/or query time.
The supported operations are listed below for images and videos.
These operations were chosen as samples or that were needed for some of the applications we were building.
Given that we wrap around OpenCV for performing operations to videos and images, any operation supported by OpenCV can be easily added.
More operations will be added in the future.

The operations block is composed by an array of operation objects.
Each operation has a `type` (which is the name of the operation) and the parameters for that operation.

At least one operation must be defined in the array.

All the parameters are required for each operation.
Below we detail the operations supported for Images (`AddImage`, `FindImage`), those operations supported for Videos (`AddVideo`, `FindVideo`), and the operations that are common for both Images and Videos.

## Common Operations and Parameters
The following are operations common for both images (`AddImage`, `FindImage`) and videos (`AddVideo`, `FindVideo`):

| Operation (`type`) | Parameters |
| ------------------ | ---------- |
| `threshold` | * `value`: is the value of the threshold above which all pixel values are returned and below which all pixel values are zeroed. |
| `crop` | * `x`, `y`: specify the starting position x and y, respectively.<br>* `width`, `height`: specify the width and height of the crop, respectively. |
| `resize` | * `width`, `height`: give the new expected width and height of the image, performs bilinear interpolation entity (entities). |


## Extra Operations for Images
The following are operations are supported for images (`AddImage`, `FindImage`):

| Operation (`type`) | Parameters |
| ------------------ | ---------- |
| `flip` | `code`: Follows OpenCV convention: 0 for Vertical flip, >0 for Horizontal flip, <0 for both. |
| `rotate` | `angle`: Rotation angle.<br>`resize`: Flag to specify whether the image will be resized so that the rotated image fits (resize = true), or if the image will be kept the same size (resize = false). |
| `userOp` *(See [User Defined Operations](../guides/remote-and-udf-operations/User-Defined-Operations.md) for more details)* | `options`:<br> * `id`: Mandatory parameter which specifies the operation to be executed and should be a key in the functions parameter of the settings.json file.<br> * `format`: (Optional) Specifies the format in which the image is required. Default is `jpg`.<br> * `port`: Port on which the message queue will be listening and writing. |
| `remoteOp` *(See [Remote Operations](../guides/remote-and-udf-operations/Remote-Operations.md) for more details)* | `url`: URL for the API endpoint<br>`options`:<br> * `id`: Mandatory parameter which specifies the operation to be executed and should be same as the file name used by the python script on the remote server.<br> * `format`: (Optional) Specifies the format in which the image is required. Default is `jpg`. |

## Extra Operations for Videos
The following are operations are supported for videos (`AddVideo`, `FindVideo`):

| Operation (`type`) | Parameters |
| ------------------ | ---------- |
| `interval` | `unit`: number of frames in between<br>`start`: first frame<br>`end`: last frame |

## Examples

Example for resizing to 200 x 200 and returning pixels above 155.
```JSON
    "operations": [
        {
            "type": "resize",
            "width": 200, "height": 200,
        }
        {
            "type": "threshold",
            "value": 155,
        }
    ],
```

Example for cropping a 15 x 10 box starting at pixel location (255, 155) and returning pixels above 155.
```JSON
    "operations": [
        {
            "type": "crop",
            "x": 255, "y": 155,
            "width": 15, "height": 10,
        },
        {
            "type": "threshold",
            "value": 155,
        }
    ],
```

This example flips the frames of a video horizontally and picks every 5th frame starting from the 20th frame and stopping at the 200th frame.
```JSON
    "operations": [
        {
            "type": "flip",
            "code": 1 // Horizontal Flip
        },
        {
          "type": "interval", // Interval Operation
          "start": 20,        // Start from the 20th frame
          "stop": 200,        // Stop at the 200th frame
          "step": 5           // Pick 1 every 5 frames.
        }
    ],
```
