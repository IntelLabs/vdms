# AddEntity Command

This method adds a new entity and can check if the
entity already exists via the constraints block.
But this command is not for updating an existing node.


## Parameters

* `class`: Name of entity class (limited to 16 characters).
* [optional] [`_ref`](../Keywords.md#keywords): for reference


## Blocks
* [optional] [`properties`](../blocks/Block-properties.md)
* [optional] [`constraints`](../blocks/Block-constraints.md)
* [optional] [`link`](../blocks/Block-link.md)


The `class` is essentially a way
to group similar kinds of entities. This is based
on the schema defined for some application. Simple examples of classes can
be "Person", "Car", Animal", "BrainScan".

These are case sensitive and are matched exactly when searching for entities of the same class.

Properties are optional but recommended to help
with searches.
Using the constraints block in `AddEntity` makes it a conditional add. This
means, we will first search for a node meeting the constraint(s) given in
the block and add this node only if no node is found. Unique is assumed
here, which means, if more than one node match the constraint(s) provided, no
change is made to the database and an error code is returned. In case `_ref`
was provided, this will cause an exception.


## Examples

Add the entity representing `Jane Doe`:
```JSON
"AddEntity" : {
    "class" : "Person",
    "_ref": 1234,
    "properties" : {
        "Name" : "Jane Doe",
        "Gender": "F",
        "Email": "jane.doe@xyz.com"
    }
}
```

Conditionally add the entity representing Jane Doe, assuming the "Email" property uniquely identifies a person.
```JSON
"AddEntity" : {
    "class" : "Person",
    "_ref": 1234,
    "constraints" : {
        "Email": [ "==", "jane.doe@xyz.com" ],
    },
    "properties": {
        "Name" : "Jane Doe",
        "Ethnicity" : "Asian",
        "Gender": "F",
        "Email": "jane.doe@xyz.com"
    }
}
```

Add a "Visit" entity that has a start and end time.
```JSON
"AddEntity" : {
    "class" : "Visit",
    "_ref": 4567,
    "properties" : {
        "start": {"_date": "Mon Aug 7 10:59:24 PDT 2017"},
        "end": {"_date": "Mon Aug 7 12:59:24 PDT 2017"}
    }
}
```
