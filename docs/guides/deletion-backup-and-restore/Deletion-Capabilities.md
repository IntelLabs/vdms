## Deletion Capabilities

This version of VDMS provides two methods to delete content from the VDMS. Currently, bot methods are active deletion methods that require the user to perform a query that identifies which entities and images should be deleted. This implementation introduces two new special property keywords "\_deletion" and "\_expiration" to the API.

### \_deletion
The \_deletion query allows a user to delete the content within VDMS that is associated with a find query (FindImage, FindEntity, FindDescriptor). In order to use this query, the
A user can arbitrarily delete all of the nodes that correspond with a query using the \_deletion keyword. When the deletion flag is used the conditions are not checked, but are needed to be accepted by the query parser. The following query will remove all contents since the query is searching for all database entries that have transpired since t = 0 (epoch).
```python
# Add Entity
props = {}
props["name"] = "Luis"
props["lastname"] = "Ferro"
props["age"] = 27
addEntity = {}
addEntity["properties"] = props
addEntity["class"] = "test_class"
query = {}
query["AddEntity"] = addEntity
all_queries = []
all_queries.append(query)
response, res_arr = db.query(all_queries)

# Find Entity and delete it
findEntity = {}
query_results = {}
query_results['list'] = ["name", "age", "lastname"]
findEntity["results"] = query_results
constraints = {}
constraints["_deletion"] = ["==", 1]
constraints["lastname"] = ["==", "Ferro"]
findEntity["constraints"] = constraints
query={}
query["FindEntity"] = findEntity
res, res_arr = db.query([query])
print(db.get_last_response_str())
```

### \_expiration
The \_expiration keyword allows for the removal of data that is based on the time of creation and a relative parameter that indicates the lifetime of value. Similar to the \_deletion keyword, a query must be performed to remove data from the database. A user must add the \_expiration keyword as a property along with a value that corresponds with the minimum number of seconds the data should reside within VDMS.

When the \_expiration keyword is used when adding data, the keyword \_creation is automatically generated for the newly added data. Both of the keywords \_expiration and \_creation are properties that can be retrieved in results of find queries.

The follow code snippet shows the creation of an entity with the \_expiration flag:
```python
addEntity = {}
addEntity["_ref"] = 2
addEntity["class"] = "sample"
props = {}
props["_expiration"] = 10
addEntity["properties"] = props
query = {}
query["AddEntity"] = addEntity
res, res_arr = db.query([query])
```

When the user wishes to remove data that has expired, the user must perform a query that searches for data with an \_expiration timestamp constraint that is prior ("<") the current time. Greater than (">") queries with \_expiration constraints will return results if database entries are present, but these entries will not be removed from VDMS.

The following code snippet shows the query used to remove the previously inserted data from VDMS. However, this query will only remove data at least 10 seconds after the data is inserted. A query run before the data expires will not return any entities - thus no entries will be removed from VDMS.
```python
query = {}
findEntity = {}
query_results = {}
query_results['list'] = ["_expiration", "_creation"]
findEntity["results"] = query_results
constraints = {}
constraints["_expiration"] = ["<", calendar.timegm(time.gmtime())]
findEntity["constraints"] = constraints
query["FindEntity"] = findEntity
print(query)
res, res_arr = db.query([query])
```
