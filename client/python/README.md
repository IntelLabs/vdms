# VDMS Client Python Module

This is the client module for Intel Lab's Visual Data Management System (VDMS).
It provides utilities for interacting with the VDMS server.

## Installing
To install this client, run the following:
```bash
pip install vdms
```

## Connect to VDMS Server
To connect to a VDMS server, you must provide the `HOST` and `PORT` where the VDMS server is deployed.
By default, the `HOST` and `PORT` are `localhost` and `55555`, respectively.
```python
import vdms
db = vdms.vdms()
db.connect()
```
or you can specify other values for `HOST` and `PORT`:
```python
import vdms
db = vdms.vdms()
db.connect(HOST, PORT)
```

For more information, visit [VDMS Documentation](https://intellabs.github.io/vdms/).
