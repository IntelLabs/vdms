## VDMS Configuration File

VDMS uses a configuration file (written in JSON) that can be specified when starting the server by using the -cfg flag:

    ./vdms -cfg config-vdms.json

If no configuration file is specified, VDMS will try to open the default file (config-vdms.json), and will fail to initiate if the file is not found.

## Parameters

All the parameters in the configuration file are optional, as VDMS has default values for all of them.

| Param | Explanation | Default |
| ----- | ----------- | ------- |
| autodelete_interval_s | Time interval (in seconds) to delete specifc entries (Should be greater than 0) | -1 |
| autoreplicate_interval | Time interval to backup the DB folder (Should be greater than 0) | -1 |
| aws_log_level | This parameter is optional and it is used when "storage_type" parameter is set to "aws" value, it is used to control the level of verbosity of AWS logging system. The acccepted values are "off", "fatal", "error", "warn", "info", "debug", "trace" | off |
| backup_flag | Boolean whether to use the auto-replication thread | false |
| backup_path | Path to store the backup DB | `db_root_path` |
| blobs_path | Path to folder where blobs will be stored | blobs (db/blobs) |
| bucket_name | Bucket name for AWS storage | vdms_bucket |
| db_root_path | Path to the root folder where all filed/objects will be stored | db |
| descriptors_path | Path to folder where descriptors will be stored | descriptors (db/descriptors) |
| endpoint_override | Server address (including scheme and port number) to override the S3 storage address server. This parameter is valid when the "storage_type" parameter is set to "aws" value and "use_endpoint" parameter is set to true. | `http://127.0.0.1:9000` when "use_endpoint" parameter is set to true and the "storage_type" parameter is set to "aws" |
| expiration_time | Time interval (in seconds) to automatically delete entries | |
| flinng_cells_per_row | For the FLINNG indexing for descriptors, controls the number of bits in the distance sensitive LSH vector for each row| 1000 |
| flinng_hashes_per_table | For the FLINNG indexing for descriptors, controls the number of hash functions to be used per table for group testing| 12 |
| flinng_num_hash_tables | For the FLINNG indexing for descriptors, controls the number of hash tables (permutations) to be used for the dataset for group testing| 10 |
| flinng_num_rows | For the FLINNG indexing for descriptors, controls the number of distance sensitive LSH vectors | 3 |
| hnsw_efConstruction | For the HNSW indexing for descriptors, controls the breadth of the search during the index construction phase| 96 |
| hnsw_efsearch | For the HNSW indexing for descriptors, controls the breadth of the search during the search query| 64 |
| hnsw_M | For the HNSW indexing for descriptors, controls the maximum number of neighbors that each descriptor can have at each layer| 48 |
| images_path | Path to folder where images (all formats) will be stored | images (db/images) |
| ivf_nlist | For the IVF FLAT indexing for descriptors, specify the number of partitions to create using the k-means algorithm| 16 |
| k8s_container | Boolean whether to use Kubernetes orchestration | false |
| max_simultaneous_clients | Number of max simultaneous connections open| 500 |
| pmgd_num_allocators | Number of allocators when creating a new PMGD graph (this will only be used when creating a new graph, and ignored if the graph already exist) | 1 |
| pmgd_path | Path to folder where PMGD graph will be stored | db |
| port | TCP port for incoming connections| 55555 |
| proxy_host | Address of the proxy (optional). Example: "a.proxy.from.intel.com" | |
| proxy_port | Port number of the proxy. This parameter is needed when "proxy_host" parameter is set | |
| proxy_scheme | Scheme used by the proxy, accepted values are "http" and "https". This parameter is needed when "proxy_host" and "proxy_port" parameters are set | |
| query_handler | Specifies the query handler to use. Accepted values: `pmgd` (PMGD), or `neo4j` (Neo4j) | pmgd |
| replication_time |  | -1 |
| storage_type | Database storage type. Accepted values: `local` (local storage), or `aws` (AWS S3) | local |
| tmp_path| Path to the temporary directory (optional) | `/tmp/tmp` |
| unit | Unit of the `autoreplicate_interval` variable. Accepted values: `h` (hour), `m` (minute), or `s` (seconds) | s |
| use_endpoint | Boolean whether to use an AWS storage mocking server (MinIO). This parameter is valid when the "storage_type" parameter is set to "aws" value | false |


## Config File Example

```JSON
// VDMS Config File
// This is the run-time config file
// Sets database paths and other parameters
{
    "port": 55555,
   "cert_file": "cert.pem",
   "key_file": "key.pem",
   "ca_file": "ca.pem",
    "autoreplicate_interval":-1, // it should be > 0
    "unit":"s",
    "max_simultaneous_clients": 100,
    // "backup_path":"backups_test", // set this if you want different path to store the back up file
    "db_root_path": "db",
    "backup_flag" : "false",
    "storage_type": "local", //local, aws, etc
    "bucket_name": "vdms_bucket",
    "more-info": "github.com/IntelLabs/vdms",
    "use_endpoint": false, // storage_type is set to local
    "endpoint_override": "http://127.0.0.1:9000",// Format "scheme://ip:port"
    "k8s_container": false,
    "proxy_host": "a.proxy.from.intel.com",
    "proxy_port": 912,
    "proxy_scheme": "http", // [http|https] valid values,
    "aws_log_level": "debug", // [off|fatal|error|warn|info|debug|trace]
    "tmp_path": "/tmp/tmp"
}
```


## Default Directories Structure

By default, VDMS will create a directory structure as follows:

```
db
├── blobs
├── descriptors
├── graph
│   ├── allocator.jdb
│   ├── edges.jdb
│   ├── graph.jdb
│   ├── indexmanager.jdb
│   ├── journal.jdb
│   ├── nodes.jdb
│   ├── stringtable.jdb
│   └── transaction.jdb
└── images
    ├── jpg
    ├── png
    └── tdb
```