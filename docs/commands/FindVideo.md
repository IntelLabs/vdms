# FindVideo Command

Works in a similar manner as `FindEntity` with the exception of returning video
blobs and allowing the possibility of applying operations before returning
those video blobs.

## Parameters

* [optional] [`_ref`](../Keywords.md#keywords): for reference
* [optional] `codec`: Indicates the codec to be transcoded.
* [optional] `container`: Indicates the container used for the video file
* [optional] `unique`: [True or False]. Indicates whether a single element is expected to match that condition.


## Blocks

* [required] [results](../blocks/Block-results.md)
* [optional] [`operations`](../blocks/Block-operations.md)
* [required] [`constraints`](../blocks/Block-constraints.md)
* [optional] [`link`](../blocks/Block-link.md)


## Example
Find "The God Father" video and apply operations `resize` and `threshold`.
```JSON
{
    "FindVideo": {
        "constraints" : {
            "name" : ["==", "The God Father"],
        },
        "operations": [
            {
                "type": "resize",
                "w": 200,
                "h": 200,
            },
            {
                "type": "threshold",
                "value": 155,
            }
        ]
    }
}
```
