# NeoClassifyDescriptor Command

***WIP: Support in future release***

***Note:*** This is largely the same as the legacy `ClassifyDescriptor` command.

When a new feature vector is extracted from an image, a query to VDMS can be issued to classify that feature vector based on the indexed features in VDMS. VDMS can respond to the query with the label associated with that descriptor.

The bare minimum requirement for this call is the descriptor set we should look to match, the number of closest labels to look for, and descriptor blob of matching dimensions.


## Parameters
* `target_data_type`: Currently this supports images (`img`) and metadata (`md_only`). If an image is specified, an associated blob must be uploaded with it.
* [optional] `target_format`: Specifies the targeted file format for uploaded images, e.g. `jpg`
* `cypher`: Specially formatted cypher is used to specify the metadata to be either uploaded or retrieved. See below for more details.


## Blocks
* [optional] [`operations`](../blocks/Block-operations.md)

