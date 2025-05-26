# Auto-Deletion of Expired Data

VDMS 2.1 introduced the concept of data expiration in which database entries could include a property that indicated a timestamp at which this data no longer needed to be stored. This implementation was active deletion in which a user was required to perform a query via a client connection to invoke this expiration query. While this is preferred by some users, some users have no need of expired data and would prefer for the data to automatically be deleted. Autodeletion of expired data is enabled by modifying the config-vdms.json file that is found in the same directory as the vdms server executable to include the field autodelete_interval_s. This parameter indicates the interval at which expired data is purged from the database.

**Original config-vdms.json**
```json
{
    "port": 55555,
    "max_simultaneous_clients": 100,
    "db_root_path": "db",
    "more-info": "github.com/IntelLabs/vdms"
}
```
<br>


**Auto-Deletion config-vdms.json**
```json
{
    "port": 55555,
    "max_simultaneous_clients": 100,
    "db_root_path": "db",
    "more-info": "github.com/IntelLabs/vdms",
    "autodelete_interval_s" : 20
}
```
With this configuration, expired data is removed from the database every 20 seconds.

## DeleteExpired  Command
The autodeletion feature utilizes a new VDMS command DeleteExpired, that removes all database entries from the database that have an expiration timestamp in the past. To improve performance of removing expired data, a special priority queue is created to store pointers to all of the new database entries with an expiration time. This ensures an exhaustive search isn't needed to examine every entry with an expiration time.  The following command.
```python
[
    {
        "DeleteExpired": {
            "results": {
                "list": ["_expiration"]
            }
        }
    }
]
```

**Python example**
```python
db = vdms.vdms()
db.connect()

result_list = ["_expiration"]
results = {}
results["list"] = result_list

deleteExpired = {}
deleteExpired["results"] = results

query = {}
query["DeleteExpired"] = deleteExpired

res, arr = db.query([query])
```
<br>


## Auto Auto-Deletion
A new feature enabled by VDMS allows for all ingested data to have a set expiration duration. This removes the requirement that a programmer specify the expiration duration for every database entry. However, a programmer can modify this default expiration duration by explicitly providing a `_expiration` property for an entry. To enable the auto auto-deletion feature, it is necessary to modify the config-vdms.json file to add the `expiration_time` field that corresponds to the default expiration duration value in seconds.

**Auto-Deletion config-vdms.json**
```json
{
    "port": 55555,
    "max_simultaneous_clients": 100,
    "db_root_path": "db",
    "more-info": "github.com/IntelLabs/vdms",
    "autodelete_interval_s" : 20
}
```

**Auto Auto-Deletion config-vdms.json**
```json
{
    "port": 55555,
    "max_simultaneous_clients": 100,
    "db_root_path": "db",
    "more-info": "github.com/IntelLabs/vdms",
    "autodelete_interval_s" : 20,
    "expiration_time" : 3600,
}
```


With this configuration, every new database entry will automatically be deleted after an hour (60 seconds * 60 minutes).

