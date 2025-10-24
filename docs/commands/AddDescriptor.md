# AddDescriptor Command

VDMS natively supports high-dimensional feature vector operations allowing efficient similarity searches, particularly useful in ML pipelines. Feature vectors or descriptors are intermediate results of various machine learning or computer vision algorithms when run on visual data. These vectors can be labeled and classified to build search indexes. VDMS does not extract descriptors but once they are available, it can store, index, and search for similarity.

Before inserting any descriptor into VDMS, we need to create the DescriptorSet. When creating a DescriptorSet, we can specify the name that will be assigned to that set, the metric that will be used for searching, and the dimensions of the descriptors that will be inserted in the set (see [`AddDescriptorSet`](AddDescriptorSet.md) command).

A descriptor, associated with a set, can then be added as below.
The bare minimum requirement for this call is the name of the set and a descriptor blob of matching dimensions (to the set).

Properties can be added to the Descriptor, as well as `link` block can be
used to specify relationships in a similar way `AddImage` supports.
Link block can be used for creating connections between the inserted object
(Image, Entity, etc) and other, previously referenced, object.


## Parameters

* `set`: Name of the set.
* [optional] `label`: Label of the descriptor. If unspecified, the system will store it as "None"
* [optional] [`_ref`](../Keywords.md#keywords): for reference
* [optional] `batch_properties`: List of properties for each descriptor being inserted using batches.<br>
>***NOTE:*** This is mutually exclusive with the use of the `properties` block. If you wish to encode a batch of descriptors into the associated blob, you must use the batch properties field. The server side will verify that the number of properties in the list correctly correlates to the size/length of the descriptor batch.


## Blocks
* [optional] [`properties`](../blocks/Block-properties.md).<br>
>***NOTE:*** This is mutually exclusive with the `batch_properties` field, and can only be used when uploading a single descriptor at a time.
* [optional] [`link`](../blocks/Block-link.md)


## Examples

### Add Single Descriptor
Insert a Descriptor for someone's face and associate with a previously inserted image (`_ref` = 1)
```python
[
    {
        "AddDescriptor": {
            "set": "party_faces",       # Specify the name of the DescriptorSet
            "label": "Ricky Fort"       # Assign a label to the descriptor
            "properties": {             # Add application-specific properties
                "gender": "M",
                "age": 45
            }
            "link": {                   # We can create a connection between the image
                "ref": 1                # and the descriptor.
            }
        }
    }
]
```
<br>


### Add Batch of Descriptors
Insert Descriptors using batch size of 4
```python
[
    {
        "AddDescriptor":
        {
            "set": "party_faces_batch_set",         # Specify the name of the DescriptorSet
            "batch_properties": [                   # Add properties for batch
                {
                    "faceID": 1,
                    "gender": "M",
                    "age": 45
                },
                {
                    "faceID": 2,
                    "gender": "F",
                    "age": 35
                },
                {
                    "faceID": 3,
                    "gender": "M",
                    "age": 20
                },
                {
                    "faceID": 4,
                    "gender": "F",
                    "age": 20
                }
            ]
        }
    }
]
```
<br>

