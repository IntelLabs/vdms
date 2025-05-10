## AddDescriptorSet Command

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
used to specify relationships in a similar way AddImage supports.
Link block can be used for creating connections between the inserted object
(Image, Entity, etc) and other, previously referenced, object.

### Parameters

* name: name of the set.
* dimensions: number of dimensions of the feature vector
* [optional] engine: underlying implementation for indexing and computing distances (FaissFlat by Default)
  * TileDBDense, TileDBSparse, FaissFlat, FaissIVFFlat, FaissHNSWFlat, Flinng
* [optional] metric: method used to calculate distances (L2 by default)
  * "L2" (euclidean distance), "IP" (inner product)
* [optional] _ref: for reference.

### Blocks
* [optional] properties
* [optional] link


### Examples

Insert a DescriptorSet for doing face matching
```json
{
    "AddDescriptorSet": {
        "engine": "FaissFlat",
        "metric": "L2",
        "name": "pretty_faces",
        "dimensions": 128
    }
}
```
