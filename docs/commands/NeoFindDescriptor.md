# NeoFindDescriptor Command

***Note:*** This is largely the same as the legacy FindDescriptor call however does not currently support links through the _ref block.

We can perform queries to find descriptors that have some specific properties. We can also perform queries to find descriptors that are similar to some "query" descriptor. Descriptor blob(s) are returned unless "blob" is explicitly set to false in the "results" block (please see "results" block).

### Parameters

* set: name of the set.
* [optional] k_neighbors: number of neighbors to be returned.

### Blocks
* [optional] constraints
* [optional] results

**Note**: A Visual Descriptor search can be constraint by either a query blob
and k_neighbors, OR by constraints, not both.
This functionality is not implemented yet.

### Examples
For instance, suppose that we want to get all the descriptors from people that have certain age, as we are interested in studying some particular characteristics of those descriptors.

We can run that query by doing:

    "NeoFindDescriptor ": {
        "set": "party_faces",           // Specify the descriptor set
        "constraints": {
            "age": [">=", "30"]          // We want only those which correspond to people
        },                              // 30 years or more older.
        "results": {
            "list": ["age", "gender"]   // We want some properties to be returned
        }
    }

In this case, VDMS will return:

    "NeoFindDescriptor ": {
        "status": "success",
        "entities": [
            {                       // Let's say this represents Ricky Fort
                "age": 45,
                "gender": "M"
            },
            {                       // Let's say this represents Regina George
                "age": 32,
                "gender": "F"
            }
        ]
    }
    + blob                          // The blob is returned using the client library.
                                    // In this case, it will be an array with the values :
                                    // [2.12, 3.4, 56.3, ...](Ricky) and
                                    // [16.5, 4.7, 28.1, ...](Regina)

Suppose that we want to get all the descriptors that are most similar to:

    [16.6, 4.9, 27.8, ...] (query descriptor)

We can run the following query:

    "NeoFindDescriptor ": {
        "set": "party_faces",  // Specify the descriptor set

        // We specify that we want only the two nearest neighbor
        "k_neighbors": 2,

        "results": {
            "list": [
                "_label",      // We want the label assigned to that descriptor
                "gender",      // its gender
                "_distance"    // and its distance to the query descriptor
            ],

            // We specify that in this case,
            // we don't want the actual descriptor to be returned.
            "blob": False

        }
    }

    + blob          // The blob is passed using the client library.
                    // In this case, it will be an array with the values :
                    // [16.6, 4.9, 27.8, ...] (the query descriptor)

Naturally, the closest neighbor to that query descriptor will be the one that corresponds to Regina George's face.

In this case, VDMS will return:

    "NeoFindDescriptor ": {
        "status": "success",
        "entities": [
            {
                "_label": "Regina George",
                "gender": "F",
                "_distance": 34.342
            },
            {
                "_label": "Karen Smith",
                "gender": "F",
                "_distance": 287.345
            }
        ]
    }

