# VDMS: Your Favourite Visual Data Management System

VDMS is a storage solution for efficient access of big-”visual”-data that aims
to achieve cloud scale by searching for relevant visual data via visual
metadata stored as a graph and enabling machine friendly enhancements to
visual data for faster access.  We use an in-persistent-memory graph database
developed in our team called Persistent Memory Graph Database (PMGD) as the
metadata tier and we are exploring the use of an array data manager, TileDB
and other formats for images, visual descriptors, and videos as part of our
Visual Compute Library (VCL). VDMS is run as a server listening for client
requests and we provide client side bindings to enable communication between (
python, C++) applications and the server. Hence, it also has a Request Server
component defined to implement the VDMS API, handle concurrent client
requests, and coordinate the request execution across its metadata and data
components to return unified responses. This project aims to research the use
of a scalable multi-node graph based metadata store as part of a hierarchical
storage framework specifically aimed at processing visual data, and also it
includes an investigation into the right hardware and software optimizations
to store and efficiently access large scale (pre-processed) visual data.

## Motivation

Data access is swiftly becoming a bottleneck in visual data processing,
providing an opportunity to influence the way visual data is treated in the
storage system. To foster this discussion, we identify two key areas where
storage research can strongly influence visual processing run-times:
efficient metadata storage and new storage formats for visual data. We
propose a storage architecture designed for efficient visual data access
that exploits next generation hardware and give preliminary results showing
how it enables efficient vision analytics.


## Get Started

To get started, take a look at the [INSTALL.md](INSTALL.md) file, where
you will find instructions on how to install and run the server.

Also, visit our [wiki](https://github.com/IntelLabs/vdms/wiki)
to learn more about the VDMS API, and take a look at some of
the examples/tutorials.



## Deletion Capabilities

This version of VDMS provides two methods to delete content from the VDMS. Currently, bot methods are active deletion methods that require the user to perform a query that identifies which entities and images should be deleted. This implementation introduces two new special property keywords "\_deletion" and "\_expiration" to the API.

##### \_deletion
The \_deletion query allows a user to delete the content within VDMS that is associated with a find query (FindImage, FindEntity, FindDescriptor). In order to use this query, the 

##### \_\_expiration\_\_
The \_\_expiration\_\_ keyword allows for the removal of data that is based on the time of creation and a relative parameter that indicates the lifetime of value. Similar to the \_\_deletion\_\_ keyword, a query must be performed to remove data from the database. A user must add the \_\_expiration\_\_ keyword as a property along with a value that corresponds with the minimum number of seconds the data should reside within VDMS. 

When the \_expiration keyword is used when adding data, the keyword \_creation is automatically generated for the newly added data. Both of the keywords \_expiration and \_creation are properties that can be retrieved in results of find queries. 

The follow code snippet shows the creation of an entity with the \_expiration flag

addEntity = {} \
addEntity["_ref"] = 2 \
addEntity["class"] = "sample" \
props = {} \
props["\_\_expiration\_\_"] = 10 \
addEntity["properties"] = props \
query = {} \
query["AddEntity"] = addEntity \
res, res_arr = db.query([query]) \

When the user wishes to remove data that has expired, the user must perform a query that searches for data with an \_expiration timestamp constraint that is prior ("<") the current time. Greater than (">") queries with \_expiration constraints will return results if database entries are present, but these entries will not be removed from VDMS.

The following code snipper shows the query used to remove the previously inserted data from VDMS. However, this query will only remove data at least 10 seconds after the data is inserted. A query run before the data expires will not return any entities - thus no entries will be removed from VDMS.

query = {} \
findEntity = {} \
query_results = {} \
query_results['list'] = ["_expiration", "_creation"] \
findEntity["results"] = query_results \
#findEntity["results"] = results \
constraints = {} \
constraints["_expiration"] = ["<", calendar.timegm(time.gmtime())] \
#constraints["_creation"] = [">", 0] \
findEntity["constraints"] = constraints \
query["FindEntity"] = findEntity \
print(query) \
res, res_arr = db.query([query]) \


## Compilation
This version of VDMS treats PMGD as a submodule so both libraries are compiled at one time. After entering the vdms directory, the command "git sumodule update --init --recursive" will pull pmgd into the appropriate directory. Futhermore, Cmake is used to compile all directories. When compiling on a target with Optane persistent memory, use the command set

mkdir build && cd build && cmake -DCMAKE_CXX_FLAGS='-DPM' .. && make -j<<number of threads to use for compiling>>

For systems without Optane, use the command set
mkdir build && cd build && cmake .. && make -j<<number of threads to use for compiling>>

## Academic Papers

Conference | Links, Cite | Description
------------ | ------------- | -------------
Learning Systems @ NIPS 2018 | [Paper](https://export.arxiv.org/abs/1810.11832), [Cite](https://dblp.uni-trier.de/rec/bibtex/journals/corr/abs-1810-11832) | Systems for Machine Learning [Workshop](http://learningsys.org/nips18/cfp.html) @ NIPS
HotStorage @ ATC 2017 | [Paper](https://www.usenix.org/conference/hotstorage17/program/presentation/gupta-cledat), [Presentation](https://www.usenix.org/conference/hotstorage17/program/presentation/gupta-cledat), [Cite](https://www.usenix.org/biblio/export/bibtex/203374)| Positioning Paper at USENIX ATC 2017 Workshop
