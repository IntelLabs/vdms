### FindVideo Command

Works in a similar manner as FindEntity with the exception of returning video
blobs and allowing the possibility of applying operations before returning
those video blobs.

### Parameters

* [optional] [[_ref|Keywords]]: for reference
* [optional] codec: indicates the codec to be transcoded.
* [optional] container: indicates the container used for the video file
* [optional] unique: [True or False]. Indicates whether a single element is expected to match that condition.

### Blocks

* [optional] [[results|Block-results]]
* [optional] [[operations|Block-operations]]
* [optional] [[constraints|Block-constraints]]
* [optional] [[link|Block-link]]

#### Example

    {
      "FindVideo": {
          "constraints" : {
              "name" : ["==", "The God Father"},
          },
          "operations": [
              {
                  "type": "resize",
                  "w": 200,
                  "h": 200,
              },
              {
                  "type": "threshold",
                  "value": 155,
              }
          ]
      }
    }
