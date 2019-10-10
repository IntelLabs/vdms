# VDMS Docker

Here are the instructions to build a docker image of the VDMS Server
after pre-populating it with the YFCC100M metadata.

The build command is: 

    docker build --build-arg=http_proxy --build-arg=https_proxy  --build-arg=no_proxy -t intellabs/vdms:yfcc100m .

To run the docker image as a container, include the --net flag.
This flag is needed as the server will be accepting connections on the default VDMS port (55555).

    // Run the image interactively 
    docker run -it --net=host intellabs/vdms:yfcc100m

    // on MacOS, use:

    docker run --rm -p 8888:8888 -p 55555:55555 -it intellabs/vdms:yfcc100m


