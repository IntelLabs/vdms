# ClassifyDescriptor Command

When a new feature vector is extracted from an image, a query to VDMS can be issued to classify that feature vector based on the indexed features in VDMS. VDMS can respond to the query with the label associated with that descriptor.

The bare minimum requirement for this call is the descriptor set we should look to match, the number of closest labels to look for, and descriptor blob of matching dimensions.


## Parameters
* `set`: Name of the set.
* `k_closest`: How many nearest neighbors to look at before returning a label


## Examples

```JSON
    {
        "ClassifyDescriptor": {
            "set": "party_faces",       // Specify the name of the DescriptorSet
            "k_closest": 1              // Classify using one nearest neighbor
        }
    }
    + blob
```

This returns the result as follows:
```JSON
"ClassifyDescriptors": {
    "status": "success",
    "label": "Ricky Fort"
}
```
