# CSV Plugin

Ease of usability is one important design goal of VDMS.
We realize many users have data in CSV or tabular format and want to ingest this data into VDMS.
Prior to VDMS v2.4.0, a user would create functions to parse their data and convert it to JSON queries for ingestion.
Now, a user can use the CSV Plug-in to ingest their data into VDMS using the C++ Client.
Currently, this capability is not available with the Python client.

This document provides details for using the CSV Plugin for C++ Client. This document includes:

[Basic Building Blocks](#basic-building-blocks)

   * [constraints](#constraints)
   * [operations](#operations)
   * [properties](#properties)
   * [rectangle](#rectangle)
<br>

[Insert Commands](#insert-commands)

  * [AddBoundingBox](#addboundingbox)
  * [AddConnection](#addconnection)
  * [AddDescriptor](#adddescriptor)
  * [AddDescriptorSet](#adddescriptorset)
  * [AddEntity](#addentity)
  * [AddImage](#addimage)
  * [AddVideo](#addvideo)
<br>


## Basic Building Blocks

### Constraints
Building block constraints are specified using a prefix 'cons_' followed by am index. This prefix is case sensitive.

cons_1 specifies the first constraint of the constraints json block.

Examples of different types of properties are:

| cons_1 | cons_2 | cons_3 |
| ------ | ------ | ------ |
| "age>=20,<=90" | gender==M | name==Poonam |

<br>

The table will result in the following constraints block:
```json
"constraints": {
   "age": [ ">=", 20, "<=", 90 ],
   "gender": [ "==", "M" ],
   "name": [ "==", "Poonam" ]
}
```
<br>


### Operations
In a CSV file, operations are provided as a column.  The column name is case-sensitive and are specified using `ops_` prefix followed by the operation type.
Examples of each operation are in the following table:

| Operation | Column Name | Example Value | Description |
| --------- | ----------- | ------------- | ----------- |
| Threshold | ops_threshold | 150 | Threshold value where all pixels above provided value are returned; Otherwise pixel values are zero |
| Crop | ops_crop | "255,224,15,10" | Comma delimited values for `x`, `y`, `width`, `height`, respectively. |
| Resize | ops_resize | "200,175"  | Comma delimited values for `width`, and `height`, respectively. |
| Flip | ops_flip | 1 | Follows OpenCV convention: 0 for Vertical flip, >0 for Horizontal flip, <0 for both. |
| Rotate | ops_rotate | "30,TRUE" | Comma delimited values for angle of rotation, and a flag indicating whether the image will be resized so that the rotated image fits (resize = true), or if the image will be kept the same size (resize = false). |
| Interval | ops_interval | "10,50,2" | Comma delimited values for first frame, last frame, and step size (frames in between), respectively. |

<br>

If all above operations are provided in a single query, the JSON equivalent is:
```json
"operations": [
   {
      "type": "crop",
      "x": 255,
      "y": 224,
      "width": 15,
      "height": 10,
   },
   {
      "type": "resize",
      "width": 200,
      "height": 175,
   },
   {
      "type": "flip",
      "code": 1
   },
   {
      "type": "rotate",
      "angle": 30,
      "resize": true
   },
   {
      "type": "interval",
      "start": 10,
      "stop": 50,
      "step": 2
   },
   {
      "type": "threshold",
      "value": 150,
   }
]
```
<br>


### Properties
Building block properties are specified using a prefix `prop_`. For any property with date as value is specified with a prefix `prop_date:`. This prefix is case sensitive.

prop_propertyname specifies that it represents a property with `propertyname` as the key of the properties json block. Properties can have string, numeric, Boolean, and alphanumeric values.

Only date/time is written in a specific format that is YYYY-MM-DDThh:mm:ssTZD. Currently all values in the date format should be specified.

Examples of different types of properties are :

| prop_name | prop_age | prop_date:DoB | prop_hasdog | prop_gender | prop_weight | prop_email  | prop_address |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Poonam | 26 | 1995-01-29T18:45:12-08:00 | TRUE | F | 55.9 | mydummymail@gmail.com | 90 kings land, Jodhpur |


The above table will result in the following json:
```json
"properties" : {
   "DoB" : {"_date" : "1995-01-29T18:45:12-08:00"},
   "age" : 26,
   "gender" : "F",
   "hasdog" : true,
   "name" : "Poonam",
   "weight" : 55.9,
   "email" : "mydummymail@gmail.com",
   "address" : "90 kings land, Jodhpur"
}
```
<br>


### Rectangle
This is used to set the coordinates for a bounding box.
To provide a rectangle value, in the cell provide 4 comma delimited numeric values representing x, y, w, and h respectively.
<br>


## Insert Commands

### AddBoundingBox
Bounding Boxes are a special kind of entity, similar to images.
The AddBoundingBox call allows applications to add a region of interest
to VDMS, where the entity added has a preset class of BoundingBox.
For more details about the command, see [AddBoundingBox](../commands/AddBoundingBox.md).

Let's take the following JSON queries for ingesting two Bounding Boxes as an example:
```python
[
   {
      "AddBoundingBox" : {
         "rectangle" : {
            "x" :120 ,"y" :50 ,"w" :40 ,"h" :40
         },
         "properties" : {
            "name" : "jowe",
            "id": 1
         }
      }
   },
   {
      "AddBoundingBox" : {
         "rectangle" : {
            "x" :100 ,"y" :20 ,"w" :20 ,"h" :40
         },
         "properties" : {
            "name" : "suitcase",
            "id": 2
         }
      }
   }
]
```
<br>

The equivalent CSV file is as follows:
```
RectangleBound,prop_name,prop_id
"120,50,40,40",jowe,1
"100,20,20,40",suitcase,2
```
<br>

In this CSV, the first row provides the column names needed for the AddBoundingBox command and each additional row represents details for one bounding box.
The first column is mandatory while the remaining columns are based on the other information needed for the entries.

The following table provides details for the CSV format:

| Column Name | Description | Has Position Constraint? | Column Position | Is field mandatory? |
| ----------- | ----------- | ------------------------ | --------------- | ------------------- |
| RectangleBound | This column is required and provides a rectangular region of interest in the format `x,y,w,h` | Yes | 1 | Yes |
| prop_name| This column is user-defined. It specifies the value for property named `name` | No | - | No |
| prop_id | This column is user-defined. It specifies the value for property named `id` | No | - | No |
| cons_1 | This column is user-defined. It specifies the first constraint needed to find the image to link to Bounding Box | No | - | No |

**Note:** If cell value is empty it means that column name is not applicable for that Bounding Box data.
<br>


### AddConnection
This command helps an application associate two entities
with specific class on the relationship and properties if needed.
The source and destination references (ref1 and ref2) corresponds to
some references created with the AddEntity command or
FindEntity command (using "_ref").
For more details about the command, see [AddConnection](../commands/AddConnection.md).

Let's take the following JSON queries for adding a connection between two entities as an example:
```json
[
   {
      "FindEntity" :
      {
            "_ref" : 1,
            "class" : "Person",
            "constraints" :
            {
               "id" : ["==", 1]
            }
      }
   },
   {
      "FindEntity" :
      {
            "_ref" : 3,
            "class" : "Person",
            "constraints" :
            {
               "id" : ["==", 2]
            }
      }
   },
   {
      "AddConnection" :
      {
         "class": "BloodRelation",
         "ref1" : 1,
         "ref2" : 3,
         "properties" : {
            "type": "brother"
         }
      }
   }
]
```
<br>

The equivalent CSV file is as follows:
```
ConnectionClass,Person@id,Person@id,prop_type
BloodRelation,1,2,brother
```
<br>

In this CSV, the first row provides the column names needed for the AddConnection command and each additional row represents a connection between two entities.
The first column is mandatory while the remaining columns are based on the other information needed for the entries.

The following table provides details for the CSV format:

| Column Name | Description | Has Position Constraint? | Column Position | Is field mandatory? |
| ----------- | ----------- | ------------------------ | --------------- | ------------------- |
| ConnectionClass | This column is required and provides a class for the connection | Yes | 1 | Yes |
| Person@id | This column is user-defined. It specifies the constraints to find the first entity for a connection. In this example, the first entity must have class `Person` with property `id` equal to 1  | No | - | No |
| Person@id | This column is user-defined. It specifies the constraints to find the second entity for a connection. In this example, the second entity must have class `Person` with property `id` equal to 2 | No | - | No |
| prop_type| This column is user-defined. It specifies the value for property named `type` | No | - | No |

**Note:** If cell value is empty it means that column name is not applicable for that connection.
<br>


### AddDescriptor
VDMS natively supports high-dimensional feature vector operations allowing efficient similarity searches, particularly useful in ML pipelines. Feature vectors or descriptors are intermediate results of various machine learning or computer vision algorithms when run on visual data. These vectors can be labeled and classified to build search indexes. VDMS does not extract descriptors but once they are available, it can store, index, and search for similarity.
For more details about the command, see [AddDescriptor](../commands/AddDescriptor.md).

Let's take the following JSON queries for ingesting a descriptor as an example:
```JSON
[
   {
      "AddDescriptor" : {
         "set" : "Test_14096",
         "label" : "Rocky",
         "properties" : {
            "age" : 34,
            "gender" : "M"
         }
      }
   },
   {
      "AddDescriptor" : {
         "set" : "Test_14096",
         "label" : "Carolyn",
         "properties" : {
            "age" : 67,
            "gender" : "F"
         }
      }
   }
]

[descriptor1,descriptor2]
```
<br>

The equivalent CSV file is as follows:
```
DescriptorClass,label,prop_age,prop_gender,inputdata
Test_14096,Rocky,34,M,blob_1.txt
Test_14096,Carolyn,67,F,blob_2.txt
```
<br>

In this CSV, the first row provides the column names needed for the AddDescriptor command and each additional row represents details for one descriptor.
The first column is mandatory while the remaining columns are based on the other information needed for the descriptors.

The following table provides details for the CSV format:

| Column Name | Description | Has Position Constraint? | Column Position | Is field mandatory? |
| ----------- | ----------- | ------------------------ | --------------- | ------------------- |
| DescriptorClass | This column is required and provides the name of the DescriptorSet to add the descriptor | Yes | 1 | Yes |
| label | This column specifies the label of the descriptor | No | - | No |
| prop_age| This column is user-defined. It specifies the value for property named `age` | No | - | No |
| prop_gender| This column is user-defined. It specifies the value for property named `gender` | No | - | No |
| inputdata| This column is required as it provides the file path which contains the actual blob for the descriptor/feature vector | No | - | Yes |

**Note:** If cell value is empty it means the column is not applicable for that Descriptor.
<br>


###  AddDescriptorSet
A DescriptorSet is a group of descriptors with a fixed number of dimensions that are
the result of the same algorithm for feature extraction. For instance, we can
create a DescriptorSet and insert multiple descriptors obtained by using
OpenFace (128 dimensions), and then index and perform matching operations over
those descriptors.
For more details about the command, see [AddDescriptorSet](../commands/AddDescriptorSet.md).

Let's take the following JSON queries for adding two descriptor sets as an example:
```JSON
[
   {
      "AddDescriptorSet" : {
         "name": "pretty_faces",
         "dimensions": 128,
         "metric": "L2",
         "engine": "FaissFlat",
         "properties": {
            "algorithm": "OpenFace"
         }
      }
   },
   {
      "AddDescriptorSet" : {
         "name": "Test_14096",
         "dimensions": 1024,
         "metric": "IP",
         "engine": "FaissIVFFlat",
         "properties": {
            "algorithm": "random"
         }
      }
   }
]
```
<br>

The equivalent CSV file is as follows:
```
DescriptorType,dimensions,distancemetric,searchengine,prop_algorithm
pretty_faces,128,L2,FaissFlat,OpenFace
Test_14096,1024,IP,FaissIVFFlat,random
```
<br>

In this CSV, the first row provides the column names needed for the AddDescriptorSet command and each additional row represents details for one descriptor set.
The first column is mandatory while the remaining columns are based on the other information needed for the descriptors.

The following table provides details for the CSV format:

| Column Name | Description | Has Position Constraint? | Column Position | Is field mandatory? |
| ----------- | ----------- | ------------------------ | --------------- | ------------------- |
| DescriptorType | This column is required and provides the name of the DescriptorSet | Yes | 1 | Yes |
| dimensions | This column is required and specifies the number of dimensions of the feature vector | No | - | Yes |
| distancemetric | This column is specifies the method used to calculate distances (IP or L2) | No | - | No |
| searchengine | This column specifies the underlying implementation for indexing and computing distances (TileDBDense, TileDBSparse, FaissFlat, or FaissIVFFlat) | No | - | No |
| prop_algorithm | This column is user-defined. It specifies the value for property named `algorithm` | No | - | No |

**Note:** If cell value is empty it means the column is not applicable for that DescriptorSet.
<br>


### AddEntity
Adds a new entity to VDMS.
For more details about the command, see [AddEntity](../commands/AddEntity.md).

Let's take the following JSON queries for adding three entities as an example:
```json
[
   {
      "AddEntity" : {
         "_ref" : 1,
         "class" : "Person",
         "constraints" : {
            "age" : [">=", 10, "<=", 30],
            "name" : ["==", "Ali"]
         },
         "properties" : {
            "DoB" : {"_date" : "2018-02-27T13:45:12-08:00"},
            "age" : 30,
            "gender" : "M",
            "hasdog" : true,
            "name" : "Ali"
         }
      }
   },
   {
      "AddEntity" : {
         "_ref" : 2,
         "class" : "Person",
         "constraints" : {
            "age" : ["<=",10],
            "name" : ["==","alex"]
         },
         "properties" : {
            "age" : 10,
            "gender" : "M",
            "hasdog" : false,
            "name" : "alex"
         }
      }
   },
   {
      "AddEntity" : {
         "_ref" : 3,
         "class" : "Person",
         "constraints" : {
            "age" : [">=",10],
            "name" : [ "==","Shri"]
         },
         "properties" : {
            "DoB" :{"_date" : "2018-02-27T13:45:12-08:00"},
            "age" : 33,
            "gender" : "F",
            "name" : "Shri"
         }
      }
   }
]
```
<br>

The equivalent CSV file is as follows:
```
EntityClass,prop_name,prop_age,cons_1,prop_date:DoB,cons_2,prop_hasdog,prop_gender
Person,Ali,30,"age>=10,<=30",2018-02-27T13:45:12-08:00,name==Ali,TRUE,M
Person,alex,10,age<=10,,name==alex,FALSE,M
Person,Shri,33,age>=10,2018-02-27T13:45:12-08:00,name==Shri,,F
```
<br>

In this CSV, the first row provides the column names needed for the AddEntity command and each additional row represents details for one entity.
The first column is mandatory while the remaining columns are based on the other information needed for the entities.

The following table provides details for the CSV format:

| Column Name | Description | Has Position Constraint? | Column Position | Is field mandatory? |
| ----------- | ----------- | ------------------------ | --------------- | ------------------- |
| EntityClass | This column is required and provides the class for the entity | Yes | 1 | Yes |
| prop_name | This column is user-defined. It specifies the value for property named `name` | No | - | No |
| prop_age | This column is user-defined. It specifies the value for property named `age` | No | - | No |
| cons_1 | This column is user-defined and specifies a constraint for a conditional add | No | - | No |
| prop_date:DoB | This column is an user-defined date for property named `DoB` | No | - | No |
| cons_2 | This column specifies the second constraint to use for the conditional add | No | - | No |
| prop_hasdog | This column is user-defined. It specifies the value for property named `hasdog` | No | - | No |
| prop_gender | This column is user-defined. It specifies the value for property named `gender` | No | - | No |

**Note:** If cell value is empty it means the column is not applicable for that entity.
<br>


### AddImage
Adds a new image to VDMS.
For more details about the command, see [AddImage](../commands/AddImage.md).

The following JSON query adds an image and performs the threshold and resize operations:
```JSON
[
   {
      "AddImage" : {
         "format" : "png",
         "properties" : {
            "type" : "scan",
            "part" : "brain"
         },
         "operations" : [
            {"type" : "threshold", "value" : 150},
            {"type" : "resize", "width" : 200, "height" : 200}
         ]
      }
   }
]

[imageblob]
```
<br>

The equivalent CSV file is as follows:
```
ImagePath,prop_type,prop_part,format,ops_threshold,ops_resize
/C:/Documents/images/click1.jpg,scan,brain,png,150,"200,200"
```
<br>

In this CSV, the first row provides the column names needed for the AddImage command and each additional row represents details for one image.
The first column is mandatory while the remaining columns are based on the other information needed for the images.

The following table provides details for the CSV format:

| Column Name | Description | Has Position Constraint? | Column Position | Is field mandatory? |
| ----------- | ----------- | ------------------------ | --------------- | ------------------- |
| ImagePath | This column is required and provides the file path which contains the actual blob for the image | Yes | 1 | Yes |
| prop_type | This column is user-defined. It specifies the value for property named `type` | No | - | No |
| prop_part | This column is user-defined. It specifies the value for property named `part` | No | - | No |
| format | This column specifies the format used to store the image [jpg, png, tdb, bin] | No | - | No |
| ops_threshold | This column is optional and specifies the value to use for the threshold operation | No | - | No |
| ops_resize | This column is optional and specifies the `width,height` to use for the resize operation | No | - | No |

**Note:** If cell value is empty it means the column is not applicable for that Image.
<br>


### AddVideo
This call allows an application to add (and preprocess) a video in VDMS.
The video blob is an encoded binary array using any of the supported containers
or encodings.
For more details about the command, see [AddVideo](../commands/AddVideo.md).

The following JSON query adds a Video to VDMS:
```JSON
[
   {
      "AddVideo" : {
         "codec" : "h264",
         "container" : "avi",
         "properties" : {
            "name" : "memories"
         },
         "index_frames" : true,
         "operations" : [
            {"type" : "threshold", "value" : 200}
         ]
      }
   }
]
```
<br>

The equivalent CSV file is as follows:
```
VideoPath,prop_name,format,compressto,ops_threshold,frameindex
C:/Documents/Videos/v1.mp4,memories,avi,h264,200,TRUE
```
<br>

In this CSV, the first row provides the column names needed for the AddVideo command and each additional row represents details for one video.
The first column is mandatory while the remaining columns are based on the other information needed for the descriptors.

The following table provides details for the CSV format:

| Column Name | Description | Has Position Constraint? | Column Position | Is field mandatory? |
| ----------- | ----------- | ------------------------ | --------------- | ------------------- |
| VideoPath | This column is required and provides the file path which contains the actual video | Yes | 1 | Yes |
| prop_name | This column is user-defined. It specifies the value for property named `name` | No | - | No |
| format | This column specifies the container used for the video file [mp4, avi, mov] | No | - | No |
| compressto | This column specifies the codec to be transcoded [xvid, h264, h263] | No | - | No |
| ops_threshold | This column is optional and specifies the value to use for the threshold operation | No | - | No |
| frameindex | This column is optional and triggers key-frame index extraction on the video | No | - | No |
| fromserver | This column is not in the example but can be used to read a file from the server | No | - | No |

**Note:** If cell value is empty it means the column is not applicable for that Video.
<br>

