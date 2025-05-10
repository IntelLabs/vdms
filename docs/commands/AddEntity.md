### AddEntity Command

Adds a new entity. This method can check if the
entity already exists constraints block is provided. But this command
is not for updating an existing node (we will add that later). The local id
allows an application to chain this command with following operations.

### Parameters

* class: name of entity class [limited to 16 characters].
* [optional] _ref: for reference.

### Blocks
* [optional] properties
* [optional] constraints
* [optional] link

The **class** is essentially a way
to group similar kind of entities. This is based
on the schema defined for some application. Simple examples of classes can
be "Person", "Car", Animal", "BrainScan".

These are case sensitive and are matched exactly when searching for entities
of the same class.

Properties are optional but recommended, at least properties that can help
with searches.
Using the constraints block in AddEntity makes it a conditional add. This
means, we will first search for a node meeting the constraint(s) given in
the block and add this node only if no node is found. Unique is assumed
here, which means, if more than one node match the constraint(s) provided, no
change is made to the database and an error code is returned. In case _ref
was provided, this will cause an exception.


### Examples

    // From the retail schema above, add the entity representing Jane Doe

    "AddEntity" : {
        "class" : "Person",
        "_ref": 1234,
        "properties" : {
            "Name" : "Jane Doe",
            "Gender": "F",
            "Email": "jane.doe@xyz.com"
        }
    }

    // From the retail schema above, conditionally add the entity
    // representing Jane Doe, assuming the "Email" property uniquely identifies
    // a person.

    "AddEntity" : {
        "class" : "Person",
        "_ref": 1234,
        "constraints" : {
            "Email": [ "==", "jane.doe@xyz.com" ],
        }
        "properties" : {
            "Name" : "Jane Doe",
            "Ethnicity" : "Asian",
            "Gender": "F",
            "Email": "jane.doe@xyz.com"
        }
    }

    // From the retail schema above, add a visit for Jane Doe. We will see an
    // example of how to connect this entity to the previous one.

    "AddEntity" : {
        "class" : "Visit",
        "_ref": 4567,
        "properties" : {
             "start": {"_date": "Mon Aug 7 10:59:24 PDT 2017"}
             "end": {"_date": "Mon Aug 7 12:59:24 PDT 2017"}
             }
    }

    // Add a "Visit", that has a start and end time.

    "AddEntity" : {
        "class" : "Visit",
        "properties" : {
             "start": {"_date": "Mon Aug 7 10:59:24 PDT 2017"}
             "end": {"_date": "Mon Aug 7 12:59:24 PDT 2017"}
             }

    }