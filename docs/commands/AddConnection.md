### AddConnection Command

This command helps an application associate two entities
with specific class on the relationship and properties if needed.
The source and destination references (ref1 and ref2) corresponds to
some references created with the AddEntity command or
FindEntity command (using "_ref").
Once this connection between the two entities is added, this
connection or link can then be used to find the related entities when
searching. We allow properties to be specified for relationships too. A
typical example would be the timestamp of when a relationship was established.


### Parameters

* ref1: name of entity class.
* ref2: name of entity class.
* class: name of entity class.

### Blocks
* [optional] properties

Just like entities, relations between two entities
can also be grouped by a class
and it has the same characteristics as the one described above for AddEntity.

Properties are optional but recommended, at least properties that can help
with searches.

### Examples

    // Add the entity representing Jane Doe

    "AddEntity" : {
        "class" : "Person",
        "_ref": 1,
        "properties" : {
            "Name" : "Jane Doe",
            "Ethnicity" : "Sagitarian",
            "Gender": "F",
            "Email": "jane.doe@xyz.com"
        }
    }

    // Conditionally add the entity representing Jon Doe.

    "AddEntity" : {
        "class" : "Person",
        "_ref": 2,
        "properties" : {
            "Name" : "Jon Doe",
            "Ethnicity" : "Milkiwayian",
            "Gender": "M",
            "Email": "jon.doe@xyz.com"
        }
    }

    // Create a connection

    "AddConnection" : {
        "class" : "BestFriendsForever",
        "ref1" : 1,
        "ref2" : 2,
        "properties" : {
            "place_met" : "Interestelar Park",
            "date" : {"_date", "Mon Aug 7 10:59:24 PDT 2017"}
        }
    }
