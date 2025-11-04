# UpdateConnection Command

Updates existing connection(s). This command provides ways to find one or more
existing connections and then lets an application add, change or remove properties.
The local id allows an application to chain this command with following operations
or associate with a previous search


## Parameters
* [optional] `class`: Name of connection class or empty string to search across multiple classes.
* [optional] [`_ref`](../Keywords.md#keywords): for reference
* [optional] `ref1`: Reference for source/origin entities for finding this set of
connections
* [optional] `ref2`: Reference for end/destination entities for finding this set of connections


## Blocks
* [optional] [`properties`](../blocks/Block-properties.md)
* [optional] [`remove_props`](../blocks/Block-properties.md)
* [optional] [`constraints`](../blocks/Block-constraints.md)

***NOTE:*** At least one of `_ref` or `class` + `constraints` are required for searching. **

If `_ref` is provided, it essentially refers to the search results of a previous
`FindConnection` or `AddConnection`.
Since the Update call just results in updating the
property values inline within those connections, we keep the same `_ref` value.
The call can result in an update to multiple connections depending the search criteria.
Unique is not assumed to be true in this case.

Class and property keys are case sensitive and are matched exactly when searching for connections or updating properties.

If search results are empty, this will cause an exception.


## Examples

Remove the `Since` property, assuming the "Area" property uniquely identifies a view.
```JSON
"UpdateConnection" : {
    "class" : "Views",
    "_ref": 1234,
    "constraints" : {
        "Area": [ "==", "Clothing" ],
    },
    "remove_props" : [ "Since" ]
}
```

