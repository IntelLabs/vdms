# Remote Storage in VDMS

VDMS allows data files (images, videos, etc) to be stored in 2 locations: either local disk storage (default), or remote cloud storage.  Currently remote cloud storage is only supported on AWS S3, or other remote storage with an identical API (such as MinIO).

## Requirements

* AWS S3 SDK
* MinIO (optional mock S3 server)

## Configuration

The decision to use local or remote storage is determined from the `config-tests.json` upon server startup.

`./vdms -cfg config-tests.json`

In order to use AWS S3 storage, two lines must be specified in the config file as below: **storage_type** and **bucket_name**
```JSON
{

    "port": 55557,
    "db_root_path": "test_db_1",
    "storage_type": "aws", //local, aws, etc
    "bucket_name": "minio-bucket",
    "more-info": "github.com/IntelLabs/vdms"
}
```

## Setup

VDMS is currently set to use MinIO as a mock AWS S3 server by default.  To setup MinIO, follow the steps below.  Any bucket name can be used, and long as it matches the value specified in config-tests.json

```bash
# Download MinIO, create bucket
$ curl -L -o /vdms/minio https://dl.min.io/server/minio/release/linux-amd64/minio
$ chmod +x /vdms/minio
$ mkdir -p /vdms/minio_files/minio-bucket
```

Credentials must be supplied either in the ~/.aws/credentials file, or be present as environment variables.  An example using the default MinIO credentials is below:

```
# ~/.aws/credentials
aws_access_key_id = minioadmin
aws_secret_access_key = minioadmin
```

Finally, run the server with the specified storage location:

```bash
$ ./minio server ./minio_files &
```

## Switch from MinIO to AWS

In order to switch between using MinIO and actual AWS S3, only a single line of code needs to be changed.  For MinIO, override the endpoint URL to the correct value (this will be printed to stdout when launching MinIO).  Otherwise, to use AWS S3, comment this line out in `src/vcl/RemoteConnection.cc`:

`clientConfig.endpointOverride = "http://127.0.0.1:9000"; //override the endpoint to use MinIO`
