# User Defined Operations in VDMS
This submodule is required to execute user defined operations (UDF) in VDMS using message queues (Support only available for images). Although shipped with VDMS, this submodule can be run independently and interacts with VDMS using message queues.

## Requirements
- Python 3 or higher
- Following python libraries
    - opencv-python
    - zmq

## UDF Definition
Any operation can be added to the module by creating a python file and adding it to the `functions` folder. All related files for the UDF should be stored in the folder `functions/files`. The operaion file should follow the following setup to define a `run` function that the interface file for VDMS will use;
```
def run(settings, message, input_params):

    # message: The inputfile and other parameters sent from VDMS
    # settings: System specific settings for the udf
    # input_params: Any parameters required by the UDF to run

    # Create outputfile
    # Read from inputfile
    '''
    The UDF logic goes here
    '''
    # Return outputfile
```

Update the `settings.json` file with the following parameters;

```
{
    "opfile": "/tmp/tmp_op_file",                              # Location where the outputfile temporary file will be stored
    "port": 5555,                                              # Port on which the message queue will be listening and writing
    "functions" : {
        "facedetect" : "facedetect",                           # Key value pair for the UDFs. 'key' is the UDF id and 'value' is the filename of the UDF.
        "flip": "flip",
        "carcount": "carcount",
        "activityrecognition": "activityrecognition"
    }
}
```

## Setup
1. Either run from the location where you have the VDMS repo or just copy the `user_defined_operations` directory to wherever you want to run the UDFs, but ensure that it is on the same system as VDMS. 
2. Create your UDFs as python scripts and place them in the `user_defined_operations/functions` directory.
3. Update the `settings.json` file to include your UDF file and other necessary information.
4. Follow the following steps to run the `user_defined_operations` submodule on port <port_number>.

```
cd user_defined_operations
python3 -m venv venv
source venv/bin/activate
python3 -m pip install pip --upgrade
python3 -m pip install wheel
python3 -m pip install -r requirements.txt
python3 udf_local.py
```

## Client Query

The client query should contain the following two parameters:

- `type`: Should always be `userOp` for remote operation
- `options`: Any parameter that is required by the operation. The following three parameters are important:
    - `id`: A mandatory parameter. It specifies the operation to be executed and should be a key in the `functions` parameter of the `settings.json` file. For instance, if the key is `facedetect`, then the `id` should be `facedetect`.
    - `format`: Optional, but specifies the format in which the image is required. Default is `jpg`.
    - `port`: The port on which the message queue will be listening and writing.

```
"FindImage": {
    "format": "png",
    "constraints": {
        "category": ["==", "cars"]
    },
    "operations": [
        {
            "type": "userOp",
            "options": {
                "id": "carcount",
                "format": "png",
                "port": 5555
            }
        }
    ]
}
```

## Detailed Instructions for new UDF
We now provide an example to add a new UDF `cardetect`. The `cardetect` operation detects cars in an image and creates a rectangle around all cars. This operation requires a pretrained model available in the form of `xml` file online.

1. Copy `user_defined_operations` directory to anywhere you want but on the same server that is running VDMS. Say you copy the folder in the `home` directory. The folder structure you have now will look something like this;
```
~/
|__user_defined_operations
   |__functions
   |  |__files
   |  |  |__haarcascade_frontalface_default.xml
   |  |__facedetect.py
   |  |__flip.py
   |__README.md
   |__requirements.txt
   |__settings.json
   |__udf_local.py
```
2. Download/Copy the `cars.xml` file to the `~/user_defined_operations/functions/files`.
3. Create the `cardetect.py` file in `~/user_defined_operations/functions`.
```
import time
import cv2
from PIL import Image
import numpy as np

car_cascade_src = 'functions/files/cars.xml'

def run(settings, message, input_params):

    global car_cascade_src

    t1 = time.time()

    ipfilename = message
    format = message.strip().split('.')[-1]

    opfilename = settings["opfile"] + str(t1) + '.' + format

    img = cv2.imread(ipfilename)
    
    # These lines
    # represent the
    # code logic

    cv2.imwrite(opfilename, img)

    return (time.time() - t1), opfilename
```
4. The final directory structure would be as follows;
```
~/
|__user_defined_operations
   |__functions
   |  |__files
   |  |  |__haarcascade_frontalface_default.xml
   |  |  |__cars.xml
   |  |__facedetect.py
   |  |__flip.py
   |  |__cardetect.py
   |__README.md
   |__requirements.txt
   |__settings.json
   |__udf_local.py
```
5. Update the settings file with the new UDF information.
```
{
    "opfile": "/tmp/tmp_op_file",
    "port": 5555,
    "functions" : {
        "facedetect" : "facedetect",
        "flip": "flip",
        "cardetect": "cardetect"
    }
}
```
6. Now start the `udf_local.py` file to initiate the message queue;
```
python3 udf_local.py
```
7. Say VDMS has a database of car images that have the property `category` set as `cars`. Then you can run the `cardetect` operation on these images using the following query;
```
"FindImage": {
    "format": "png",
    "constraints": {
        "category": ["==", "cars"]
    },
    "operations": [
        {
            "type": "userOp",
            "options": {
                "port": 5555,
                "id": "cardetect",
                "format": "png"
            }
        }
    ]
}
```