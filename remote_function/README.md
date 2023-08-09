# Remote Operations in VDMS
This submodule is required to execute VDMS operation on a remote server using Flask APIs (Support only available for images). Although shipped with VDMS, this submodule can be run independently and interacts with VDMS using http APIs.

## Requirements
- Python 3 or higher
- Following python libraries
    - flask
    - cv2
    - numpy
    - skvideo.io
    - imutils

## Operation Definition
Any operation can be added to the module by creating a python file of the same name as the operation and adding it to the `functions` folder. Any operaion file should follow the following setup to define a `run` function that the endpoint will use;
```
def run(ipfilename, format, options):
    
    # ipfilename: Name of the input file to be read from
    # format: Format of the input file
    # options: Any inputs that the UDF will require from the client

    ###
    Operation logic here
    ###

    # Return OpenCV Matrix
```

## Setup
1. Copy the `remote_function` directory on the machine you want to run the remote server. Can be run on any location, independent of where VDMS is running. However, the location should be reachable from the machine that is running VDMS. You can also use `sparse-checkout` to only retrieve the `remote_function` directory from the VDMS repo.
2. Create the operation scripts as python scripts and place them in the `remote_function/functions` directory.
4. Follow the following steps to run the remote on port <port_number>.

```
cd remote_function
python3 -m venv venv
source venv/bin/activate
python3 -m pip install pip --upgrade
python3 -m pip install wheel
python3 -m pip install -r requirements.txt
python3 udf_server.py <port_number>
```

## Client Query

The client query should contain the following three parameters:

+ `type`: Should always be `remoteOp` for remote operation
+ `url`: URL for the API endpoint
+ `options`: Any parameter that is required by the operation. The following two parameters are important:
    + `id`: A mandatory parameter. It specifies the operation to be executed and should be same as the file name used by the python script on the remote server. For instance, if the filename is `facedetect.py`, then the `id` should be `facedetect`.
    + `format`: Optional, but specifies the format in which the image is required. Default is `jpg`.

```
"FindImage": {
    "format": "png",
    "constraints": {
        "category": ["==", "faces"]
    },
    "operations": [
        {
            "type": "remoteOp",
            "url": "http://<ip>/image",
            "options": {
                "id": "facedetect",
                "format": "png"
            }
        }
    ]
}
```

## Detailed Instructions for new remote operation
We now provide an example to add a new operation `cardetect` as a remote operation that would work with VDMS. The `cardetect` operation detects cars in an image and creates a rectangle around all cars. This operation requires a pretrained model available in the form of `xml` file online.

1. Copy `remote_function` directory to your remote server machine. Say the address is `my.remote.server` and you copy the folder in the `home` directory. The folder structure you have now will look something like this;
```
~/
|__remote_function
   |__functions
   |  |__files
   |  |  |__haarcascade_frontalface_default.xml
   |  |__facedetect.py
   |__README.md
   |__requirements.txt
   |__udf_server.py
```
2. Download/Copy the `cars.xml` file to the `~/remote_function/functions/files`.
3. Create the `cardetect.py` file in `~/remote_function/functions`.
```
import time
import cv2
from PIL import Image
import numpy as np

car_cascade_src = 'functions/files/cars.xml'

def run(ipfilename, format, options):

    global car_cascade_src

    img = cv2.imread(ipfilename)
    
    # These lines
    # represent the
    # code logic

    return img
```
4. The final directory structure would be as follows;
```
~/
|__remote_function
   |__functions
   |  |__files
   |  |  |__haarcascade_frontalface_default.xml
   |  |  |__cars.xml
   |  |__facedetect.py
   |  |__cardetect.py
   |__README.md
   |__requirements.txt
   |__udf_server.py
```
5. Now start the remote server at port `5010` by running;
```
python3 udf_server.py 5010
```
6. Say VDMS has a database of car images that have the property `category` set as `cars`. Then you can run the `cardetect` operation on these images using the following query;
```
"FindImage": {
    "format": "png",
    "constraints": {
        "category": ["==", "cars"]
    },
    "operations": [
        {
            "type": "remoteOp",
            "url": "http://my.remote.server:5010/image",
            "options": {
                "id": "cardetect",
                "format": "png"
            }
        }
    ]
}
```