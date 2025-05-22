# FindEntity Command

This could either return the expected number of property values or entity ids
as a list, or apply operations like `count`, `average`, `sum` and return
response. There can be multiple entities found after applying all the search
criteria (in that order). If no search conditions are provided, you can read
all entities in the system. By giving a local id, it is possible to associate
result of the search with that value and use it later. The local id allows an
application to chain this call with following operations.

**Tip:**: Choose the first property constraint carefully in order to restrict
the most number of entity returns since that one is potentially applied on an
existing index. The other constraints are applied sequentially on the returned
result set.


## Parameters
* [optional] `class`: Name of entity class.
* [optional] [`_ref`](../Keywords.md#keywords): Local id to be used as reference for a given entity.
* [optional] `unique`: [True or False]. Indicates whether a single element is expected to match that condition.


## Blocks
* [optional] [`results`](../blocks/Block-results.md)
* [optional] [`constraints`](../blocks/Block-constraints.md)
* [optional] [`link`](../blocks/Block-link.md)


If the constraints block is skipped, all entities with the given class are returned. If the class is skipped, all entities ever added to the system are returned.

If the requested response was a list with some number of given property keys, it
indicates how many values matched the search condition.
If limit is provided, the
number of entries returned will be n <= m (which is the actually matched
quantity).


## Examples
Find an entity of class Visit where its `start` property is on or after midnight of Aug 7 2017 and then look for areas in that visit (connected to the Visit entity by the connection VisitedDuring).
No constraints are placed on the
areas returned. The results block indicates that we want to see the
names of those areas.

```JSON
"FindEntity": {
    "_ref" : 1,
    "class": "Visit",
    "constraints" : {
        "start": [ ">=", "Mon Aug 7 00:00:00 PDT 2017" ],
    }
},
"FindEntity": {
    "_ref" : 2,

    "class": "Area",
    "link": {
        "direction": "out",
        "class": "VisitedDuring",
        "ref": 1,
    },
    "results": {
        "list": "Name"
    }
}
```
