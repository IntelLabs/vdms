# results

THe results block defines the information expected in the returned JSON response object when querying for something using Find* calls.

## Parameters
The following are optional parameters for the results block:

| Parameter | Description |
| --------- | ----------- |
| `list` | Used to specify a list of properties that should be returned. |
| `count` | Expected value is `""`. It returns the total number of objects that matched the search condition. |
| `sum` | Used to request a sum of the values retrieved for the given property key. Only one key is considered. The returned type can be an Integer or Float. This call will return an error if the property is not an integer or float value. |
| `average` | Used to request an average of the values retrieved for the given property key. Only one key is considered. The returned type can be an Integer or Float. This call will return error if the property is not an integer or float value. |
| `sort` | Specifies that we should sort the results by the first key provided. Once sorted, all operations on that particular find will happen on the sorted order by the key. |
| `limit` | Specifies the maximum number of elements that will be returned. This is applied to all but the `count` option when specified. |
| `return_blob` | Specifies whether you want a blob as a result of this call or not. Typically applies to images, videos, and descriptors but can also be used for entities that were stored with a blob. Default is `False` for entities and `True` for others, e.g. a blob will be returned unless this is set to `False` when asking for results in `FindImage`, `FindVideo`, and `FindDescriptors` but not with `FindEntity` where the default is `False`. |


While all the keywords listed above are optional, exactly one of `list`,
`count`, `sum`, or `average` must be specified if a "result" block is used.
If no results block is specified for a Find call, only a success or failure
status is returned and the results of the query are cached on the server if
the local id is provided in the function call.

When `sort` is specified, it takes precedence over the other operations. So list
will be returned in the sorted order based on the chosen key. From a
performance point of view, sort makes the query traverse all the results
matching the constraints before sorting.

When `limit` is specified, `list`, `average`, or `sum` are computed on the limited
number of results.
<br>


## Examples

Return the `name` and `format` properties for the first 150 results:
```JSON
"results": {
    "list": ["name", "format"],
    "limit": 150,
}
```

Return only the number of results:
```JSON
"results": {
    "count": "", // Empty string. Any other string as value will be ignored.
}
```

Return only the first 2 instances, and average the value of those 2.
```JSON
"results": {
    "average": "age",
    "limit" : 2 // only average first 2
}
```

Returns the sum of the "number_of_cars" properties for all entities:
```JSON
"results": {
    "sum": "number_of_cars",
}
```


Return only the first 150 instances, and sort them by email:
```JSON
"results": {
    "list": ["name", "format"],
    "sort" : "email",
    "limit": 150,
}
```
