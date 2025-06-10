# properties

This can be used for defining application-specific properties for images, videos, or any other entities and connections in the system.
Think of this like columns of a row in a relational database.
Properties are specified as a list of key-value pairs.
Property keys are up to 1024 ASCII characters.
<!-- TODO: Verify current limit -->
There are no real restrictions on how many properties an entity can have and whether all entities of a class have a certain property.
It can evolve as needed.
The types for property values could be integer (up to 64bit), float, string, boolean, blob, and time.

Time and blob are special properties that can be difficult to specify due to their representation as strings.
So we have the following way to help applications call them out:
```JSON
    "key3": {"_date": "Sat Nov 1 18:59:24 PDT 1930"},
    "key4": {"_blob": "ajhgjkfdhsghfdkjgdf"}
```

## Example
```JSON
    "properties": {
        "name": "scan1",
        "part": "anterior brain",
        "height": 1024,
        "aperture": 34.546,
        "date": {"_date": "Sat Jun 14"}
    }
```
