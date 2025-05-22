# FindConnection Command

This can return the expected number of property values
as a list or apply operations like `count`, `average`, `sum` on them and return
that. There can be multiple edges found after applying all the search
criteria (in that order). If no search conditions are provided, you can read
all edges in the system.

**Tip:** Choose the first property constraint carefully in order to restrict
the most number of edge returns since that one is potentially applied on an
existing index. The other constraints are applied sequentially on the returned
result set.

## Parameters

* [optional] `class`: name of edge class.
* [optional] `_ref`: Use an _ref to cache edges found with this call
* [optional] `ref1`: Use an _ref value from a previous Entity add/find to limit the starting/source entities for matching edges
* [optional] `ref2`: Use an _ref value from a previous Entity add/find to limit the ending/destination entities for matching edges


## Blocks
* [optional] [`results`](../blocks/Block-results.md)
* [optional] [`constraints`](../blocks/Block-constraints.md)


If the constraints block is skipped, all edges with the given class are returned.
If the class is skipped, all edges ever added to the system are returned.

If the requested response was a list with some number of given property keys
Indicates how many values matched the search condition.
If limit is provided, the
number of entries returned will be `n <= m` (which is the actually matched
quantity)


## Examples
To count number of edges of class Conducts:

```JSON
"FindConnection": {
    "class": "Conducts",

    "results": {
        "count" : ""
    }
}
```

To find information about edges from class Visit where its "start" property is on or after midnight of Aug 7 2017.
```JSON
"FindEntity": {
    "_ref" : 1,
    "class": "Visit",
    "constraints" : {
        "start": [ ">=", "Mon Aug 7 00:00:00 PDT 2017" ],
    }
}
```

Now find the connections that originate from the Visit entities above and have a class #To
```JSON
"FindConnection": {
    "ref1": 1,
    "class": "To",

    "results": {
        "count" : ""
    }
}
```
