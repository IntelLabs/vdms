**Note:** This API description correspond to the latest version VDMS, in the [develop branch](https://github.com/IntelLabs/vdms/tree/develop).

Once the data usage and data collection/preprocessing requirements of an
application are understood, it is time to connect the application to a VDMS
server because VDMS offers the unique capabilities of storing metadata in a
structured graph database as well as takes care of dealing with the actual
data through one unified API, described on this page. We provide C++ and
Python client libraries for linking with any application to enable VDMS
communication.

While our metadata is stored in a graph database, we have tried to simplify
the API to a level that hides some of the nitty gritties. It also opens up the
possibility of parsing this to a relational backend, though that is not our
goal in this project. We now describe the syntax and keywords for all the
currently supported commands and indicate what is optional. We also give some examples
and some explanations after each example to help understand and implement
using our API. These examples try to use some of the [schema](Schema.md) elements from the
example application schemas given earlier.

## Using the API

We define a set of JSON-based API calls that allow an application to interact
with VDMS and exploit its strengths. The table below lists the calls
provided, followed by their JSON specification. Our commands can be constructed
using some basic building blocks and return results with certain meanings.


A user application will use the VDMS API by defining metadata conforming to
the query protocol described below and passing along blobs. The client side
VDMS library provides a simple query function that accepts a JSON string with
commands and an array or vector of blobs. Internally, the library wraps the
query string and blob in a protobuf (choice for now) and sends it to VDMS. It
also receives a similarly wrapped response from VDMS and returns it to the
client. The responses will require JSON parsing on the client side, starting
with the metadata string that will indicate how to interpret blobs. We could
substitute protobuf here with explicitly sending a string followed by sending
binary data OR provide a link within the string block to indicate where to
download the actual data from. For now, we will go with the image as a blob in
the response message, and have the library in the client unpack the image
and place it in a folder. In the future, we will consider adding a flag to
indicate that VDMS will return urls and generate copies to download in the
server.
