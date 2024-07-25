# Installation
Here is the detailed process of installation of VDMS dependencies.

## Dependencies
To install VDMS, we must install the necessary dependencies via apt, github, and pip (Python 3.9+).

### Install Debian/Ubuntu Packages
Here we will install the Debian/Ubuntu packages.
```bash
sudo apt-get update -y  --fix-missing
sudo apt-get upgrade -y
sudo apt-get install -y --no-install-suggests --no-install-recommends \
    apt-transport-https automake bison build-essential bzip2 ca-certificates \
    curl ed flex g++ gcc git gnupg-agent javacc libarchive-tools libatlas-base-dev \
    libavcodec-dev libavformat-dev libavutil-dev libboost-all-dev libbz2-dev libc-ares-dev \
    libcurl4-openssl-dev libdc1394-dev libgflags-dev libgoogle-glog-dev \
    libgtk-3-dev libgtk2.0-dev libhdf5-dev libjpeg-dev libjsoncpp-dev \
    libleveldb-dev liblmdb-dev liblz4-dev libncurses5-dev libopenblas-dev libopenmpi-dev \
    libpng-dev librdkafka-dev libsnappy-dev libssl-dev libswscale-dev libtbb-dev \
    libtiff-dev libtiff5-dev libtool libzip-dev linux-libc-dev mpich \
    pkg-config procps software-properties-common swig unzip uuid-dev
```

#### **Install JPEG package**
Please install the JPEG package based on the OS platform being used:
* ***Debian 10+:*** `sudo apt-get install -y libjpeg62-turbo-dev`
* ***Ubuntu 20.04+:*** `sudo apt-get install -y libjpeg8-dev`


#### **Install Package for C++ bindings**
Please install the package for C++ bindings for libzmq (headers) based on the OS platform being used:
* ***Debian 12+:*** `sudo apt-get install -y cppzmq-dev`
* ***Debian 10-11, Ubuntu 20.04+:*** `sudo apt-get install -y libzmq3-dev`


#### **Install OpenJDK Development Kit (JDK)**
Please install the headless OpenJDK Development Kit (JDK) based on the OS platform being used:
* ***Debian 12+, Ubuntu 22.04+:*** `sudo apt-get install -y openjdk-17-jdk-headless`
* ***Debian 10-11, Ubuntu 20.04:*** `sudo apt-get install -y openjdk-11-jdk-headless`


#### **Install Parallelism library for C++ - runtime files**
Please install the package for parallelism library for C++ - runtime files based on the OS platform being used:
* ***Debian 12+, Ubuntu 22.04+:*** `sudo apt-get install -y libtbbmalloc2`
* ***Debian 10-11, Ubuntu 20.04:*** `sudo apt-get install -y libtbb2`
<br>

### Install Remaining Dependencies
Here we assume `$VDMS_DEP_DIR` is the directory for installing additional dependencies.
This directory is user-defined but here we use `/dependencies`.
These instructions assume you have full permissions to your system.
***NOTE:*** If running as ***root***, remove `sudo` where applicable.
```bash
VDMS_DEP_DIR=/dependencies  # Set to any directory
BUILD_THREADS="-j`nproc`"
mkdir -p $VDMS_DEP_DIR
```


#### Python3 Packages
It is expected that you have Python3.9 or higher installed on your system.
All python calls will use Python3.9+; therefore you may find it convenient to set alias for python.
Here we will install Python 3.12.3 from the Python website.
```bash
PYTHON_VERSION=3.12.3
curl -O https://www.python.org/ftp/python/${PYTHON_VERSION}/Python-${PYTHON_VERSION}.tgz
tar -xzf Python-${PYTHON_VERSION}.tgz
cd Python-${PYTHON_VERSION}
./configure --enable-optimizations
make altinstall
```

If you prefer, you can install the the Python 3 version available on the OS platform:
```bash
sudo apt-get install -y python3-dev python3-pip
```

***NOTE:*** If multiple versions of Python 3 are present on your system, verify you are using Python3.9 or higher. You can specify the specific verison and set an alias for `python` and/or `python3` to easily use the desired python version. This can be done using the following:
```bash
alias python=/usr/bin/python3.x
alias python3=/usr/bin/python3.x
```

Now that python is setup, now install Numpy and also install the coverage and cryptography packages if interested in running the Python unit tests.
```bash
python3 -m pip install --upgrade pip
python3 -m pip install --no-cache-dir "numpy>=1.26.0" "coverage>=7.3.1" "cryptography>=42.0.7"
```


#### **CMAKE v3.28.5**
VDMS requires CMake v3.21+.  Here we install CMake v3.28.5.
```bash
CMAKE_VERSION="v3.28.5"
git clone --branch ${CMAKE_VERSION} https://github.com/Kitware/CMake.git $VDMS_DEP_DIR/CMake
cd $VDMS_DEP_DIR/CMake
./bootstrap
make ${BUILD_THREADS}
sudo make install
```


#### **Protobuf v24.2 (4.24.2)**
Install Protobuf (C++ and Python) which requires GoogleTest and Abseil C++ as dependencies.
```bash
PROTOBUF_VERSION="24.2"
python3 -m pip install --no-cache-dir "protobuf==4.${PROTOBUF_VERSION}"

git clone -b v${PROTOBUF_VERSION} --recurse-submodules https://github.com/protocolbuffers/protobuf.git $VDMS_DEP_DIR/protobuf

cd $VDMS_DEP_DIR/protobuf/third_party/googletest
mkdir build && cd build
cmake -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DBUILD_GMOCK=ON -DCMAKE_CXX_STANDARD=17 ..
make ${BUILD_THREADS}
sudo make install

cd $VDMS_DEP_DIR/protobuf/third_party/abseil-cpp
mkdir build && cd build
cmake -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_SHARED_LIBS=ON \
    -DCMAKE_INSTALL_PREFIX=/usr/local -DABSL_BUILD_TESTING=ON \
    -DABSL_USE_EXTERNAL_GOOGLETEST=ON \
    -DABSL_FIND_GOOGLETEST=ON -DCMAKE_CXX_STANDARD=17 ..
make ${BUILD_THREADS}
sudo make install
sudo ldconfig /usr/local/lib

cd $VDMS_DEP_DIR/protobuf
cmake -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DCMAKE_CXX_STANDARD=17 -Dprotobuf_BUILD_SHARED_LIBS=ON \
    -Dprotobuf_ABSL_PROVIDER=package \
    -Dprotobuf_BUILD_TESTS=ON \
    -Dabsl_DIR=/usr/local/lib/cmake/absl .
make ${BUILD_THREADS}
sudo make install
```


#### **[OpenCV](https://opencv.org/) 4.9.0**
Below are instructions for installing ***OpenCV v4.9.0***.
```bash
OPENCV_VERSION="4.9.0"
git clone https://github.com/opencv/opencv.git $VDMS_DEP_DIR/opencv
cd $VDMS_DEP_DIR/opencv
git checkout tags/${OPENCV_VERSION}
mkdir build && cd build
cmake -D BUILD_PERF_TESTS=OFF -D BUILD_TESTS=OFF ..
make ${BUILD_THREADS}
sudo make install
```

**Note**: When using videos, and getting the following error: "Unable to stop the stream: Inappropriate ioctl for device", you may need to include more flags when compiling OpenCV. Follow these instructions ([source](https://stackoverflow.com/questions/41200201/opencv-unable-to-stop-the-stream-inappropriate-ioctl-for-device)):
```bash
sudo apt-get install -y ffmpeg
sudo apt-get install -y libavdevice-dev

cmake -D BUILD_PERF_TESTS=OFF -D BUILD_TESTS=OFF -D CMAKE_BUILD_TYPE=RELEASE -D CMAKE_INSTALL_PREFIX=/usr/local \
    -D WITH_FFMPEG=ON -D WITH_TBB=ON -D WITH_GTK=ON \
    -D WITH_V4L=ON -D WITH_OPENGL=ON -D WITH_CUBLAS=ON \
    -DWITH_QT=OFF -DCUDA_NVCC_FLAGS="-D_FORCE_INLINES" ..
make ${BUILD_THREADS}
sudo make install
```


#### **Valijson v0.6**
This is a headers-only library, no compilation/installation necessary.
```bash
VALIJSON_VERSION="v0.6"
git clone --branch ${VALIJSON_VERSION} https://github.com/tristanpenman/valijson.git $VDMS_DEP_DIR/valijson
cd $VDMS_DEP_DIR/valijson
sudo cp -r include/* /usr/local/include/
```


#### **Faiss v1.7.4**
Install the Faiss library for similarity search.
```bash
FAISS_VERSION="v1.7.4"
git clone --branch ${FAISS_VERSION} https://github.com/facebookresearch/faiss.git $VDMS_DEP_DIR/faiss
cd $VDMS_DEP_DIR/faiss
mkdir build && cd build
cmake -DFAISS_ENABLE_GPU=OFF -DPython_EXECUTABLE=/usr/bin/python3 \
    -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release ..
make ${BUILD_THREADS}
sudo make install
```


#### **FLINNG**
Install the Filters to Identify Near-Neighbor Groups (FLINNG) library for similarity search.
```bash
git clone https://github.com/tonyzhang617/FLINNG.git $VDMS_DEP_DIR/FLINNG
cd $VDMS_DEP_DIR/FLINNG
mkdir build && cd build
cmake ..
make ${BUILD_THREADS}
sudo make install
```


#### **[TileDB](https://tiledb.io/) 2.14.1**
The directions below will help you install TileDB v2.14.1 from the source.
You can also follow the directions listed [here](https://docs.tiledb.io/en/latest/installation.html).
```bash
TILEDB_VERSION="2.14.1"
curl -L -o $VDMS_DEP_DIR/${TILEDB_VERSION}.tar.gz https://github.com/TileDB-Inc/TileDB/archive/refs/tags/${TILEDB_VERSION}.tar.gz && \
cd $VDMS_DEP_DIR
tar -xvf ${TILEDB_VERSION}.tar.gz
cd TileDB-${TILEDB_VERSION}
mkdir build && cd build
../bootstrap --prefix=/usr/local/
make ${BUILD_THREADS}
sudo make install-tiledb
```


#### **AWS SDK CPP 1.11.336**
Use the following instructions to install AWS SDK for C++.
```bash
AWS_SDK_VERSION="1.11.336"
git clone -b ${AWS_SDK_VERSION} --recurse-submodules https://github.com/aws/aws-sdk-cpp ${VDMS_DEP_DIR}/aws-sdk-cpp
mkdir -p ${VDMS_DEP_DIR}/aws-sdk-cpp/build
cd ${VDMS_DEP_DIR}/aws-sdk-cpp/build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/usr/local/ -DCMAKE_INSTALL_PREFIX=/usr/local/ -DBUILD_ONLY="s3" -DCUSTOM_MEMORY_MANAGEMENT=OFF
make ${BUILD_THREADS}
sudo make install
```


#### **Autoconf v2.71**
```bash
AUTOCONF_VERSION="2.71"
curl -L -o $VDMS_DEP_DIR/autoconf-${AUTOCONF_VERSION}.tar.xz https://ftp.gnu.org/gnu/autoconf/autoconf-${AUTOCONF_VERSION}.tar.xz
cd $VDMS_DEP_DIR
tar -xf autoconf-${AUTOCONF_VERSION}.tar.xz
cd autoconf-${AUTOCONF_VERSION}
./configure
make ${BUILD_THREADS}
sudo make install
```


#### **Neo4j Client**
Below are instructions for installing ***libneo4j-omni*** which requires Peg, libcypher-parser and libedit as dependencies.
```bash
PEG_VERSION="0.1.19"
curl -L -o $VDMS_DEP_DIR/peg-${PEG_VERSION}.tar.gz https://github.com/gpakosz/peg/releases/download/${PEG_VERSION}/peg-${PEG_VERSION}.tar.gz
cd $VDMS_DEP_DIR/
tar -xf peg-${PEG_VERSION}.tar.gz
cd peg-${PEG_VERSION}
make ${BUILD_THREADS}
sudo make install

git clone https://github.com/cleishm/libcypher-parser.git $VDMS_DEP_DIR/libcypher
cd $VDMS_DEP_DIR/libcypher
./autogen.sh
./configure
sudo make install

LIBEDIT_VERSION="20230828-3.1"
curl -L -o $VDMS_DEP_DIR/libedit-${LIBEDIT_VERSION}.tar.gz https://thrysoee.dk/editline/libedit-${LIBEDIT_VERSION}.tar.gz
cd $VDMS_DEP_DIR/
tar -xzf libedit-${LIBEDIT_VERSION}.tar.gz
cd libedit-${LIBEDIT_VERSION}
./configure
make ${BUILD_THREADS}
sudo make install

git clone https://github.com/majensen/libneo4j-omni.git $VDMS_DEP_DIR/libomni
cd $VDMS_DEP_DIR/libomni
./autogen.sh
./configure --disable-werror --prefix=/usr
make clean check
sudo make install -w --debug
```
<br>

## Install VDMS
This version of VDMS treats PMGD as a submodule so both libraries are compiled at one time. After entering the vdms directory, the command `git submodule update --init --recursive` will pull pmgd into the appropriate directory. Furthermore, Cmake is used to compile all directories.
```bash
git clone -b develop --recurse-submodules https://github.com/IntelLabs/vdms.git
cd vdms
```

If your OS Platform is Debian 12+ or Ubuntu 22.04+ and you installed `openjdk-17-jdk-headless`, please modify PMGD to use this package:
```bash
sed -i "s|java-11-openjdk|java-17-openjdk|g" src/pmgd/java/CMakeLists.txt
sed -i "s|#include <stdio.h>|#include <stdio.h>\n#include <stdexcept>|" src/pmgd/test/neighbortest.cc
sed -i "s|#include <stdio.h>|#include <stdio.h>\n#include <stdexcept>|" src/pmgd/tools/mkgraph.cc
```

If your OS Platform is Debian 11 or Ubuntu 20.04, please modify file to use older FFMPEG libraries:
```bash
sed -i "s|#include <libavcodec/avcodec.h>||" include/vcl/KeyFrame.h
sed -i "s|#include <libavcodec/bsf.h>||" include/vcl/KeyFrame.h
```

When compiling on a target without Optane persistent memory, use the following:
```bash
mkdir build && cd build
cmake ..
make ${BUILD_THREADS}
cp ../config-vdms.json .
```

When compiling on a target with Optane persistent memory, use the following:
```bash
mkdir build && cd build
cmake -DCMAKE_CXX_FLAGS='-DPM' ..
make ${BUILD_THREADS}
cp ../config-vdms.json .
```

***NOTE:*** If error similar to `cannot open shared object file: No such file or directory` obtained during loading shared libraries, such as `libpmgd.so` or `libvcl.so`, add the correct directories to `LD_LIBRARY_PATH`. This may occur for non-root users. To find the correct directory, run `find` command for missing object file. An example solution for missing `libpmgd.so` and `libvcl.so` is:
```bash
find / -name "libpmgd*so*" # <Path_to_VDMS_directory>/build/src/pmgd/src
find / -name "libvcl*so*"  # <Path_to_VDMS_directory>/build/src/vcl
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:<Path_to_VDMS_directory>/build/src/pmgd/src:<Path_to_VDMS_directory>/build/src/vcl
```

