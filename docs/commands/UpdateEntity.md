# UpdateEntity Command

This command provides ways to find one or more
existing entities and then lets an application add, change or remove properties.
The local id allows an application to chain this command with following operations or associate with a previous search


## Parameters
* [optional] `class`: Name of entity class or empty string to search across multiple classes.
* [optional] [`_ref`](../Keywords.md#keywords): for reference


## Blocks
* [optional] [`properties`](../blocks/Block-properties.md)
* [optional] [`remove_props`](../blocks/Block-properties.md)
* [optional] [`constraints`](../blocks/Block-constraints.md)

***NOTE:*** At least one of _ref or class + constraints are required for searching.

If `_ref` is provided, it essentially refers to the search results of a previous
`FindEntity` or `AddEntity`.
Since the Update call just results in updating the
property values inline within those entities, we keep the same `_ref` value.

The call can result in an update to multiple entities depending on the search criteria.
Unique is not assumed to be true in this case.

Class and property keys case sensitive and are matched exactly when updating properties.

If search results are empty, this will cause an exception.


## Examples

Add the entity representing `Jane Doe`
```JSON
"AddEntity" : {
    "_ref": 1234,
    "properties" : {
        "Name" : "Janice Doe",
        "Ethnicity" : "European",
        "Email": "jane.doe@xyz.com"
    }
}
```

Remove the ethnicity, assuming the "Email" property uniquely identifies a person.
```JSON
"UpdateEntity" : {
    "class" : "Person",
    "_ref": 1234,
    "constraints" : {
        "Email": [ "==", "jane.doe@xyz.com" ],
    },
    "remove_props" : [ "Ethnicity" ]
}
```
