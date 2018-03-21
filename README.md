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

Here is our [ATC HotStorage '17 Position Paper](https://www.usenix.org/system/files/conference/hotstorage17/hotstorage17-paper-gupta-cledat.pdf).
Also, take a look at our [presentation](https://www.usenix.org/conference/hotstorage17/program/presentation/gupta-cledat).

## Get Started

To get started, take a look at the [INSTALL.md](INSTALL.md) file, where
you will find instructions on how to install and run the server.

Also, visit our [wiki](https://github.com/IntelLabs/vdms/wiki)
to learn more about the VDMS API, and take a look at some of
the examples/tutorials.

## Cite

Feel free to include our work into your research!

    @inproceedings {203374,
    author = {Vishakha Gupta-Cledat and Luis Remis and Christina R Strong},
    title = {Addressing the Dark Side of Vision Research: Storage},
    booktitle = {9th {USENIX} Workshop on Hot Topics in Storage and File Systems (HotStorage 17)},
    year = {2017},
    address = {Santa Clara, CA},
    url = {https://www.usenix.org/conference/hotstorage17/program/presentation/gupta-cledat},
    publisher = {{USENIX} Association},
    }
