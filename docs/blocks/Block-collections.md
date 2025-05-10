### collections

One of the easiest ways to search for entities is by grouping them in
collections. For example, images of all dogs could be connected to a
collection called "Dog" and then a query over the collection dog could give us
all the images that have dogs in them. So this collections block defines the
collection that will be used to associate the data, both at insertion and at
query time.

The collection block is defined as an array,
that can have N elements, with N >= 1.

#### Example

    "collections": ["cars", "trucks"]
