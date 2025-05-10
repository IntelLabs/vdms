### AddImage Command

This call allows the application to add an image in VDMS. The bare minimum
requirement for this call is the image blob or a path to an image file on disk or a mounted device.
The binary image blob should follow the json string.
While the application can choose
to associate no properties or no collections with this image, it will make it
harder to find the image later in such situations. In all scenarios, this call
will also lead to an addition of an entity with class "Image" into the
metadata database. If an operation is specified, the operation is performed
before storing the image. If a collection is defined, it will link the image
with that collection. If information is provided to link this image with some
previously searched or added entity through a user/application specified id,
then such a link is also added in the metadata database.

### Parameters

* [optional] _ref: for reference.
* [optional] from_file_path: add image using a file path on a local or mounted device.
* [optional] is_local_file: set this as true if a local copy of the file is required to be stored in VDMS data store.
* [optional] format: Specify the format used to store the image [jpg, png, tdb]. By default, the image will be kept in its original format.

### Blocks

* [optional] operations
* [optional] properties
* [optional] constraints
* [optional] link

### Inference Image Format
When ingesting an image, VDMS uses OpenCV calls to read the image and extract metadata (imread). This image is then saved (imwrite). This can result  in a change in image quality more for lossy image formats (jpg). The resulting loss has been shown to impact the accuracy of inference algorithms. If VDMS is being integrated into an inference workflow, it is recommended to use 'png', a lossless format.

### Examples
Insert an image, save it as png, and apply a threshold operation before storing.
```bash
[
    {
        "AddImage":
        {
            "format": "png",
            "operations": [
                {
                    "type": "threshold",
                    "value": 155
                }
            ],

            "properties":
            {
                "type": "scan",
                "part": "brain"
            }
        }
    }
]
+blob // the actual image as a blob
```

Insert an image linked to a patient found using FindEntity.
```bash
[
    {
        "FindEntity":
        {
            "_ref": 34,
            "class": "patient",
            "constraints" : {
                "bcr_patient_barc": [ "==", "TCGA-02-0070" ]
            }
        },
        "AddImage":
        {
            "format": "jpg",
            "operations": [
                {
                "type": "threshold",
                "value": 155
                }
            ],
            "link": {
                "class": "scan_for",
                "ref": 34,
                "direction": "in"
            }
        }
    }
]
+ blob
```

Insert an image using a file path. Image object points to the file location and uses the file for all operations. When using an operation, the file at the given location will be modified.
Note that no blob should be sent with this query option.
```json
{
    "AddImage": {
        "from_file_path": "/path/to/hiking.jpg",
        "properties" : {
            "name" : "activity_test",
            "category" : "image_path_rop"
        },
        "operations": [
            {
                "type": "rotate",
                "angle": 45,
                "resize": False
            }
        ]
    }
}
```

Insert an image using a file path. A duplicate copy is created and stored in the VDMS data store. The file at the given location is not modified or used subsequently.
Note that no blob should be sent with this query option.
```json
{
    "AddImage": {
        "from_file_path": "/path/to/hiking.jpg",
        "is_local_file": True,
        "properties" : {
            "name" : "activity_test",
            "category" : "image_path_rop"
        },
        "operations": [
            {
                "type": "rotate",
                "angle": 45,
                "resize": False
            }
        ]
    }
}
```

***Note:*** For full example, please see [Images Example](../guides/Images-Example.md), [C++ Test: add_image](https://github.com/IntelLabs/vdms/blob/master/tests/unit_tests/client_image.cc#L32), or [Python Test: insertImage](https://github.com/IntelLabs/vdms/blob/master/tests/python/TestImages.py#L79).