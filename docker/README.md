# VDMS Docker
Here are the instructions for building and running a docker image of the VDMS Server.

## Build VDMS Server
Here are a few methods for building the docker image as `vdms:latest`:

* Build image as-is:
    ```bash
    cd base
    Dockerfile | docker build -t vdms:latest .
    ```

    or build from main VDMS directory using:
    ```bash
    docker build -f docker/base/Dockerfile -t vdms:latest .
    ```

* Build image under a proxy:
    ```bash
    cd base
    Dockerfile | docker build -t vdms:latest --build-arg=http_proxy --build-arg=https_proxy .
    ```

    or build from main VDMS directory using:
    ```bash
    docker build --build-arg=http_proxy --build-arg=https_proxy -f docker/base/Dockerfile -t vdms:latest .
    ```

* Build image with Kubernetes client support:
    ```bash
    cd base
    Dockerfile | docker build -t vdms:latest --build-arg USE_K8S="on" .
    ```

    or build from main VDMS directory using:
    ```bash
    docker build --build-arg=http_proxy --build-arg=https_proxy --build-arg USE_K8S="on" -f docker/base/Dockerfile  -t vdms:latest .
    ```
<br>

## Run VDMS Server
To run the docker image as a container, include `--net=host` argument or specify a port to map to the default VDMS port (55555).

Here are a few methods for running the container:

* Run the image interactively in Host Mode:
    ```bash
    docker run -it --net=host vdms:latest
    ```
    The `--net=host` argument runs container in Host Mode which shares the container's network namespace with the host.
    When using Host Mode, it can provide near bare-metal speed but be cautious of port conflicts.

* Run the image in Host Mode but detached from your bash:
    ```bash
    docker run -d --net=host vdms:latest
    ```

* Run the image detached from your bash with host port 55555 mapped to VDMS default port.
    ```bash
    docker run -d -p 55555:55555 vdms:latest
    ```
