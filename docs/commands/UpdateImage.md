### UpdateImage Command

Updates existing image(es). This command provides ways to find one or more
existing images and then lets an application add, change or remove properties.
The local id allows an application to chain this command with following operations
or associate with a previous search

### Parameters

* [optional] _ref: for reference.

### Blocks
* [optional] properties
* [optional] remove_props
* [optional] constraints

** At least one of _ref or class + constraints are required for searching. **
If _ref is provided, it essentially refers to the search results of a previous
FindImage or AddImage. Since the Update call just results in updating the
property values inline within those entities, we keep the same _ref value. The
call can result in an update to multiple entities depending on the search
criteria. Unique is not assumed to be true in this case.

Property keys are case sensitive and are matched exactly when updating properties.

If search results are empty, this will cause an exception.


### Examples

    // From the retail schema above, add the entity representing Jane Doe

    "UpdateImage" : {
        "_ref": 1234
        "properties" : {
            "Name" : "Janice Doe",
            "Ethnicity" : "European"
        }
    }

    // From the retail schema above, remove the ethnicity,
    // assuming the "Email" property uniquely identifies
    // a person.

    "UpdateImage" : {
        "_ref": 1234
        "constraints" : {
            "Email": [ "==", "jane.doe@xyz.com" ],
        }
        "remove_props" : [ "Ethnicity" ]
    }
