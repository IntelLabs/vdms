### UpdateConnection Command

Updates existing connection(s). This command provides ways to find one or more
existing connections and then lets an application add, change or remove properties.
The local id allows an application to chain this command with following operations
or associate with a previous search

### Parameters

* [optional] class: name of connection class or empty string to search across multiple classes.
* [optional] _ref: for reference.
* [optional] ref1: reference for source/origin entities for finding this set of
connections
* [optional] ref2: reference for end/destination entities for finding this set of
connections

### Blocks
* [optional] properties
* [optional] remove_props
* [optional] constraints

** At least one of _ref or class + constraints are required for searching. **

If _ref is provided, it essentially refers to the search results of a previous
FindConnection or AddConnection. Since the Update call just results in updating the
property values inline within those connections, we keep the same _ref value. The
call can result in an update to multiple connections depending on the search
criteria. Unique is not assumed to be true in this case.

Class and property keys are case sensitive and are matched exactly when searching
for connections or updating properties.

If search results are empty, this will cause an exception.


### Examples

    // From the retail schema above, update the connection representing Jane Doe

    "UpdateConnection" : {
        "_ref": 1234
        "properties" : {
            "Name" : "Janice Doe",
            "Ethnicity" : "European"
        }
    }

    // From the retail schema above, remove the ethnicity,
    // assuming the "Email" property uniquely identifies
    // a person.

    "UpdateConnection" : {
        "class" : "Views",
        "_ref": 1234
        "constraints" : {
            "Area": [ "==", "Clothing" ],
        }
        "remove_props" : [ "Since" ]
    }

