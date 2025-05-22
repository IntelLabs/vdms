# AddDescriptorSet Command

A DescriptorSet is a group of descriptors with a fixed number of dimensions that are
the result of the same algorithm for feature extraction. For instance, we can
create a DescriptorSet and insert multiple descriptors obtained by using
OpenFace (128 dimensions), and then index and perform matching operations over
those descriptors.
This command always conditionally adds a new DescriptorSet with a given name,
i.e., will only add a new DescriptorSet if and only if that set does not exist.
This command expects the dimensionality to be specified to create the
appropriate structures on the system and to check later that added descriptors
are of the same dimensionality.

This command optionally allows the user to specify the metric that will be
used for indexing and matching operations, as well as the underlying
engine.

Properties can be added to the DescriptorSet, as well as link block can be
used to specify relationships in a similar way `AddImage` supports.
Link block can be used for creating connections between the inserted object
(Image, Entity, etc) and other, previously referenced, object.


## Parameters

* `name`: Name of the set.
* `dimensions`: Number of dimensions of the feature vector
* [optional] `engine`: Underlying implementation for indexing and computing distances. Accepted engines: `FaissFlat`(Default), `FaissHNSWFlat`, `FaissIVFFlat`, `TileDBDense`, `TileDBSparse`, `Flinng`.
* [optional] `metric`: Method used to calculate distances. Accepted metrics: `L2` (euclidean distance; Default), `IP` (inner product).
* [optional] [`_ref`](../Keywords.md#keywords): for reference


## Blocks
* [optional] [`properties`](../blocks/Block-properties.md)
* [optional] [`link`](../blocks/Block-link.md)


## Examples
Insert a DescriptorSet with dimension size of 128 for face matching using `FaissFlat` and `L2`.
```JSON
{
    "AddDescriptorSet": {
        "engine": "FaissFlat",
        "metric": "L2",
        "name": "pretty_faces",
        "dimensions": 128
    }
}
```
