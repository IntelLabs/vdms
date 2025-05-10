### constraints

Block with search criteria based on constraints defined over certain property
keys and entity class. Implicit assumption here is that the following property
constraints are all applied to the given entity class, in order, to find the
results. In the example below, the value of "key1" for an entity that meets
the criterion will be greater than equal to 20 and less than equal to 90.

**Tip:** Given our current lack of query optimizer, choose the first property
constraint carefully in order to restrict the most number of entity returns
since that one is potentially applied on an existing index. The other
constraints are applied sequentially on the returned result set.

#### Example

    "constraints": {
        "key1": [ ">=", 20, "<=", 90 ],
        "key2": [ "<=", value2 ],
        "key3": [ "==", value ]
    },

**Note** that a condition like a < key < b must be spedicied as

    "key": [ ">=", a, "<=", b ]

When querying for a time property, a "_date" keyword should be used following
the syntax:


    "time_key": ["==", {"_date": "2018-02-27T13:45:12-08:00"} ]

#### Example

    "constraints": {
        "age": [ ">=", 20, "<=", 90 ],
        "name": [ "==", "Tom" ],
        "start_date": [">=", {"_date": "2018-02-27T13:45:12-08:00"} ]
    },

