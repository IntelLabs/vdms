# Keywords

Words described next with an `_` before them are system keywords with specific
meaning for VDMS. Their usages are listed in the command examples as
appropriate.


* **`_ref`**: Local id for reference (can be defined in one call and reused in following calls). `_ref` is valid within a single transaction  and must be a number in the range [1,10000].

* **`_date`**: VDMS can parse certain date formats but the way to specify that in JSON is through a string but prefix it with this keyword. The supported format for the date is: `Mon Aug 7 12:59:24 PDT 2017`. One Example of a query using this keyword:<br>
    ```JSON
    "AddEntity" : {
        "class" : "Visit",
        "_ref": 4567,
        "properties" : {
            "start": {"_date": "Mon Aug 7 10:59:24 PDT 2017"},
            "end":   {"_date": "Mon Aug 7 12:59:24 PDT 2017"}
        }
    }
    ```

* **`_blob`**: Similar to `_date`, we want to distinguish between regular strings and blobs. This is to specify a property that goes in the metadata database. In case of images etc, we will talk about blobs but those would be provided as or returned along with the string json.

* **`_globalid`**: All entities including images, and collections have a global system id associated with them.
