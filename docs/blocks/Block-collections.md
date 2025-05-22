# collections

One of the easiest ways to search for entities is by grouping them in
collections. For example, images of all dogs could be connected to a
collection called `cars` and then a query over the collection `cars` could give us
all the images that have cars in them.
This collections block defines the
collection that will be used to associate the data, both at insertion and at
query time.

The collection block is defined as an array that can have N elements, with N >= 1.

## Example
```JSON
    "collections": ["cars", "trucks"]
```
