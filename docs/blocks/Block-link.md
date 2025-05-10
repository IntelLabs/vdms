### link

There is often a need to indicate that an entity or
image/video might be connected to another entity,
either while adding or searching (or searching and adding).
This block enables that.

#### Parameters

* [optional] **class**: class describing the relationship. If it is not specified, all relationships connected to the starting entity (entities) are explored.
* [optional] **ref**: local id of an entity (or entities) returned by another query within the transaction, using the "FindEntity", "FindImage", "AddEntity", "AddImage" commands. It is the value of the "_ref" key and indicates where to start the search from.
* [optional] **direction**: [in, out, any]. Default is "any. "in" indicates that the relationship actually ends at the starting
entity (entities).

This block has a dual meaning depending if used on a Add command or as part
of a Find command.

When used as part of an Add command (AddEntity, AddImage, etc), the link block indicates the
connection that will be created.

#### Example

    // Isolated link block
    "link": {
        "ref": 1234,
        "class": "Visit",
        "direction": "out",
    }


    // Used as part of a AddEntity command

    "AddEntity" : {
        "class" : "Person",
        "_ref": 1234,                   // Declares a reference for future use
        "properties" : {
            "Name" : "Jane Doe",
            "Ethnicity" : "Jupeterian",
            "Gender": "F",
            "Email": "jane.doe@jupiter.pla"
        }
    }
    "AddEntity" : {
        "class" : "Person",
        "link": {                       // Indicates here a connection
            "ref": 1234,                // between Jon and Jane
            "direction": "in",          // (an edge in a graph)
            "class": "Friends"
        },

        "properties" : {
            "Name" : "Jon Doe",
            "Ethnicity" : "Martian",
            "Gender": "M",
            "Email": "jon.doe@mars.pla"
        }
    }

In the case above, we are creating a connection between the inserted entities.


When used as part of a Find command (FindEntity, FindImage),
the link block indicates a condition that must be satisfy.

#### Example

    // Used as part of a FindEntity command

    "FindEntity" : {
        "class" : "Person",
        "_ref": 1234,                   // Declares a reference for future use
        "constraints" : {
            "Name" : ["==", "Jane Doe"]
        }
    }

    "FindEntity" : {
        "class" : "Person",
        "link": {                       // Indicates here a connection
            "ref": 1234,                // as a condition
            "class": "Friends"
        },

        "constraints" : {
            "Name" : ["==", "Jon Doe"]
        }
    }

In this case, the second FindEntity command aims to retrieve ONLY
those "Person" entities that has the name "Jon Doe" as the "Name" property
AND are connected to the result of the first FindEntity,
with that connection being of the class "Friends".
