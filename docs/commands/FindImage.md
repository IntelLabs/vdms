# FindImage Command

Works in a similar manner as `FindEntity` with the exception of returning image
blobs and allowing the possibility of applying operations before returning
those image blobs.


## Parameters

* [optional] [`_ref`](../Keywords.md#keywords): Local id to be used as reference for a given entity.
* [optional] `unique`: [True or False]. Indicates whether a single element is expected to match that condition.

## Blocks
* [optional] [`results`](../blocks/Block-results.md)
* [optional] [`constraints`](../blocks/Block-constraints.md)
* [optional] [`link`](../blocks/Block-link.md)
* [optional] [`operations`](../blocks/Block-operations.md)

If the `constraints` block is skipped, all entities with the given class are
returned. If the class is skipped, all entities ever added to the system are
returned.

If the requested response was a list with some number of given property keys it
indicates how many values matched the search condition.
If `limit`(m) is provided, the
number of entries returned will be `n <= m` (which is the actually matched
quantity)


## Examples

Find an image based on its property `patient_id`, apply threshold, and only return 150 results with `name` and `size` properties included.
```JSON
"FindImage": {
    "_ref": 1,
    "constraints" : {
        "patient_id": [ "==", "TCGA-02-0070" ]
    },
    "operations": [
        {
            "type": "threshold",
            "value": 45
        }
    ],
    "result": {
        "list": ["name", "size"],
        "limit": 150,
    }
}
```

Find an image connected to a `patient` entity with specific `patient_id` value.
```JSON
"FindEntity": {
    "_ref": 344554,
    "class": "patient",
    "constraints" : {
        "patient_id": [ "==", "TCGA-02-0070" ]
    }
},

"FindImage": {
    "link": {
        "ref": 344554,
    }
}
```
