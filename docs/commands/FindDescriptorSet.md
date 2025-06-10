# FindDescriptorSet Command

We can perform queries to obtain information regarding a descriptor set and to explicitly perform a store index operation for a specific index.


## Parameters
* `set`: name of the set.
* [optional] `storeIndex`: Store current indices


## Blocks
* [optional] [`constraints`](../blocks/Block-constraints.md)
* [optional] [`results`](../blocks/Block-results.md)


## Examples
This is a sample query to find a descriptor set and list the metric, dimension, and engine of the set
```json
{
  "FindDescriptorSet": {
    "set": "testFind",
    "storeIndex": true
  }
}
```

In this case, VDMS will return:
```json
[
  {
    "FindDescriptorSet": {
      "entities": [
        {
          "VD:descSetPath": "db/descriptors/testFind",
          "VD:dimensions": 128,
          "VD:engine": "FaissFlat"
        }
      ],
      "returned": 1,
      "status": 0
    }
  }
]
```
