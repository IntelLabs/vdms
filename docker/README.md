# VDMS Docker

Here are the instructions to build a docker image of the VDMS Server.

The build command is: 
    
    cd base
    Dockerfile | docker build -t intellabs-vdms:base .

    cd server
    Dockerfile | docker build -t intellabs-vdms:latest .

If you are under a proxy, use:

    cd server
    Dockerfile | docker build -t vdms:latest --build-arg=http_proxy --build-arg=https_proxy .

To run the docker image as a container, include the --net flag.
This flag is needed as the server will be accepting connections on the default VDMS port (55555).

    // Run the image interactively 
    docker run -it --net=host vdms:latest

    // or

    // Run the image and deattach it from your bash 
    docker run -d --net=host vdms:latest


