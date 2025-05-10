## VDMS Server Docker Image

To download the latest docker image and run the VDMS Server:
```bash
# On Linux to use host network
docker run --net=host -d intellabs/vdms:latest

# To map only host port 55555 to internal VDMS port 55555
docker run -p 55555:55555 -d intellabs/vdms:latest
```

The VDMS server will be listening to connection
on its default TCP port (55555).

## Persisting Data using Docker Image
In some cases, user may need for data to persist if docker container is shutdown.  In this case, use the following to run the VDMS Server:
```bash
mkdir -p db
docker run -p 55555:55555 -d --mount type=bind,source=${PWD}/db,target=/db \
-e OVERRIDE_db_root_path=/db intellabs/vdms:latest
```

This creates a directory `db` in the current directory and mounts it to `/db` within the container which is used to store VDMS data.

When using descriptors, be sure to update the index. This can be done by running the following using the VDMS Python Client before shutting down the VDMS server:
```python
import vdms

db = vdms.vdms()
db.connect()

query = [
    {
        "FindDescriptorSet": {
            "set": collection_name,
            "storeIndex": True  # Update Index
        }
    }
]
response, _ = db.query(query)
```

To start a new VDMS server, using the persisted data, use the same command as before:
```bash
docker run -p 55555:55555 -d --mount type=bind,source=${PWD}/db,target=/db \
-e OVERRIDE_db_root_path=/db intellabs/vdms:latest
```


## Environment Variable in Docker Containers
As of v2.8.0, users now have the ability to override the default parameters in config-vdms.json.
Users can specify the newer parameters as an environment variable into the docker container using the prefix `OVERRIDE_`.
For example, to override the `autodelete_interval_s` parameter, you can use the following:
```bash
docker run -d --net=host -e OVERRIDE_autodelete_interval_s=60 intellabs/vdms:v2.8.0
```

<!-- ## VDMS Server + Jupyter Notebook Image

This image contains an instance of VDMS server running,
together with the Python client module and a Jupyter Notebook
ready to use.

Once the docker instance is running, you can go to
your browser and go to localhost:8888 to connect
to the notebook. Password: vdmstest

    # On Linux
    docker run --net=host -d intellabs/vdms:notebook

    # On Mac
    docker run -p 8888:8888 -d intellabs/vdms:notebook

    # Now you can go to your browser and
    # connect to localhost:8888
    # Password: vdmstest

## VDMS Demo

This image contains an instance of the VDMS Server, a Jupyter Notebook,
and sample populated database, and some sample queries to run,
together with the Python client module and a Jupyter Notebook
ready to use.

Once the docker instance is running, you can go to
your browser and go to localhost:8888 to connect
to the notebook. Password: vdmstest

    # On Linux
    docker run --net=host -d intellabs/vdms:demo

    # On Mac
    docker run -p 8888:8888 -d intellabs/vdms:demo

    # Now you can go to your browser and
    # connect to localhost:8888
    # Password: vdmstest -->

## Other Images

VDMS Server Version 2.7.0:

    docker run --net=host -d intellabs/vdms:v2.7.0

