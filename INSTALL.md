## Installation

### Dependencies

    sudo apt-get install scons
    sudo apt-get install libjsoncpp-dev
    sudo apt-get install automake libtool curl make g++ unzip libgtest-dev
    sudo apt-get install cmake wget zlib1g-dev libbz2-dev libssl-dev liblz4-dev
    sudo apt-get install libtiff5-dev libjasper-dev libgtk-3-dev

    // Also, install one of the following for MPI
    sudo apt-get install libopenmpi-dev
    sudo apt-get install mpich

### External Libraries
* protobuf (default install location is /usr/local)
  * git clone https://github.com/google/protobuf.git
  * cd protobuf/
  * ./autogen.sh
  * ./configure
  * make && make check
  * sudo make install
  * sudo ldconfg

* valijson
  * git clone https://github.com/tristanpenman/valijson.git
  * cd valijson
  * cp -r include/* /usr/local/include (may need to run as sudo)
  * This is a headers-only library, no compilation/installation necessary

* Persistent Memory Graph Database (PMGD)
  * Download version 1.0.0 from: https://github.com/IntelLabs/pmgd/releases
  * Follow installation instructions

* Visual Compute Library
  * Download version 0.1.0 from: https://github.com/IntelLabs/vcl/releases
  * Follow installation instructions

### Requirement for Python Client

    sudo apt-get install python-pip
    pip install protobuf (may need to run as sudo)

    Add VDMS Python module to PYPATH:
    export PYTHONPATH="${PYTHONPATH}:<path_to_vdms>/client/python/vdms"
    # Example:
    export PYTHONPATH="${PYTHONPATH}:/opt/intel/vdms/client/python/vdms"

### Compilation

    git clone https://github.com/intellabs/vdms
    // Or download a release.

    cd vdms
    scons [FLAGS]

Flag | Explanation
------------ | -------------
--no-server | Compiles client libraries (C++/Python) only. (will not compile neither server not tests)
--timing    | Compiles server with chronos for internal timing.
-jX         | Compiles in parallel, using X cores
INTEL_PATH=path  | Path to the root folder containing pmgd and vcl. Default is "./" which is pmgd and vcl inside vdms folder. Example: scons INTEL_PATH=/opt/intel/

### Running The VDMS Server

The config-vdms.json file contains the configuration of the server.
Some of the parameters include the TCP port that will be use for incoming
connections, maximun number of simultaneous clients, and paths to the
folders where data/metadata will be stored.

**Note:** The folders must already exists in the filesystem.

We provide a script (run_server.sh) that will create some default directories,
corresponding the default values in the config-vdms.json.

To run the server using the default directories and port, simply run:

    sh run_server.sh

