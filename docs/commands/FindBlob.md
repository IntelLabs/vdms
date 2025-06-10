# FindBlob Command

Works in a similar manner as `FindEntity` with the exception of returning blobs.

## Parameters

* [optional] `_ref`: Local id to be used as reference for a given entity.
* [optional] `unique`: [True or False]. Indicates whether a single element is expected to match that condition.


## Blocks
* [optional] [`results`](../blocks/Block-results.md)
* [optional] [`constraints`](../blocks/Block-constraints.md)
* [optional] [`link`](../blocks/Block-link.md)


If limit (m) is provided, the number of entries returned will be n <= m (which is
the actually matched quantity)


## Examples

Find a blob based on its property `patient_id`.
```python
query = {
    "FindBlob": {
        "constraints" : {
            "patient_id": [ "==", "TCGA-02-0070" ]
        },
        "result": {
            "list": ["file", "length"],
            "limit": 1,
        }
    }
}
response, response_arr = db.query([query])
```
