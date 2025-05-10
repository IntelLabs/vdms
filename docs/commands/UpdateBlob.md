### UpdateBlob Command

Updates existing blob(s). This command provides ways to find one or more
existing blob of data and then lets an application add, change or remove properties.
The local id allows an application to chain this command with following operations
or associate with a previous search.

### Parameters

* [optional] _ref: for reference.

### Blocks
* [optional] properties
* [optional] remove_props
* [optional] constraints

** At least one of _ref or constraints are required for searching. **
If _ref is provided, it essentially refers to the search results of a previous
FindBlob or AddBlob. Since the Update call just results in updating the
property values inline within those entities, we keep the same _ref value. The
call can result in an update to multiple entities depending on the search
criteria. Unique is not assumed to be true in this case.

Property keys are case sensitive and are matched exactly when updating properties.

If search results are empty, this will cause an exception.


### Examples

The following Python example adds an audio file to VDMS and uses UpdateBlob to add the `length` property and remove the `color` property:
```python
from pathlib import Path
import vdms

db=vdms.vdms
db.connect()

audio_blob = [Path("audio_file.m4a").read_bytes()]

all_queries = [
    {
        "AddBlob":
        {

            "_ref": 12,

            "properties": {
                "Name":"blob-sample-1",
                "color": "true",
                "file":"audio"
            }
        }

    },
    {
        "UpdateBlob" : {

            "constraints": {
                "Name" : [ "==",  "blob-sample-1" ]
            },
            "remove_props" : [ "color" ],
            "properties": {
                "length" : 200
            }
        }
    }
]

response, response_arr = db.query(all_queries, audio_blob)
```
