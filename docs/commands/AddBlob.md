### AddBlob Command

This call allows the application to write different file formats in VDMS. The bare minimum
requirement for this call is the blob of data. While the application can choose
to associate no properties with the data, it will make it
harder to find the data later in such situations.
If information is provided to link this blob with some
previously searched or added entity through a user/application specified id,
then such a link is also added in the metadata database.

### Parameters
* [optional] _ref: for reference.

### Blocks
* [optional] properties
* [optional] link

### Examples
Example of inserting audio data into VDMS using AddBlob command.
```python
all_queries = [
    {
        "AddBlob": {
            "properties": {
                "patient_id": "TCGA-02-0070",
                "length": "200",
                "file": "audio"
            }
        }
    }
]

fd = open("audio_file.m4a", 'rb')
audio_blobs = [fd.read()]
fd.close()

response, response_arr = db.query(all_queries, audio_blobs)
```

Insert an audio blob linked to a patient found using FindEntity
```python
[
    {
        "FindEntity": {
            "_ref": 34,
            "class": "patient",
            "constraints" : {
                "patient_id": [ "==", "TCGA-02-0070" ]
            }
        },
        "AddBlob": {
            "properties": {
                "length": "200",
                "file": "audio"
            },
            "link": {
                "ref": 34,
                "direction": "in"
            }
        }
    }
]


fd = open("audio_file.m4a", 'rb')
audio_blobs = [fd.read()]
fd.close()

response, response_arr = db.query(all_queries, audio_blobs)
```
