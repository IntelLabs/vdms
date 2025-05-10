Each call returns a "status" with an integer value listed with its meaning in
the table below. In case a command is unsuccessful, we return an error code
and provide information on why it failed using the "info" field.

However, if there is a local id
provided in such a call, we treat it as an exception which causes the rest of
the transaction to abort. This is necessary to avoid relying on calls that
were not successful. When a transaction aborts, it undoes all the metadata
changes that were made until the point where the error occurred.

Return value | Status | Description
------------ | ------------- | -------------
0 | Success | The call was successful and performed what was expected of it
1 | Empty | A search query returned no results.
2 | Exists | A conditional add found a unique instance of the entity to exist already. If not unique, an error(-1) is returned depending on absence/presence of _ref in the call.
3 | NotUnique | If a search query wanted a unique entity but got multiple results. An error(-1) is returned depending on absence/presence of _ref in the call.
-1 | Error | There was an error and the error message indicates what


If every command within a transaction succeeded, VDMS will return one JSON
object with the corresponding status code and possible results information.
If any of the commands fails, VDMS will return a single JSON object indicating
the error status, information about what caused the failure, and the command
that caused the failure.


### Examples

    // Successful AddEntity

    "AddEntity" : {
        "status": 0
    }

    // FindEntity with using a wrong "ref"

    "FindEntity" : {
        "status": -1,
        "info": "Reference does not exist",
        "FailedCommand": {
            "FindEntity": {
                "_ref": 344554,
                "class": "patient",
                "constraints" : {
                    "bcr_patient_barc": [ "==", "TCGA-02-0070" ]
                }
        }
    }
}

