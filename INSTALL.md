# Installation
Here is the detailed process of installation of VDMS dependencies.

## Dependencies
Here we will install the Ubuntu 20.04 packages.
```bash
sudo apt-get update
sudo apt-get -y install --no-install-recommends software-properties-common
sudo add-apt-repository "deb http://security.ubuntu.com/ubuntu focal-security main"
sudo apt-get -y install --no-install-recommends apt-transport-https autoconf automake bison build-essential \
    bzip2 ca-certificates curl=7.68.0-1ubuntu2.18 ed flex g++ git gnupg-agent javacc libarchive-tools \
    libatlas-base-dev libavcodec-dev libavformat-dev libboost-all-dev libbz2-dev \
    libc-ares-dev libdc1394-22-dev libgflags-dev libgoogle-glog-dev libgtest-dev \
    libgtk-3-dev libgtk2.0-dev libhdf5-serial-dev libjpeg-dev libjpeg8-dev libjsoncpp-dev \
    libleveldb-dev liblmdb-dev liblz4-dev libopenblas-dev libopenmpi-dev \
    libpng-dev librdkafka-dev libsnappy-dev libssl-dev libswscale-dev libtbb-dev \
    libtbb2 libtiff-dev libtiff5-dev libtool maven mpich openjdk-11-jdk-headless \
    pkg-config python3-dev python3-pip unzip
pip3 install --no-cache-dir "numpy>=1.23.2" "setuptools>=65.5.1"
```
### Clone/Download Dependencies
Here we clone the repositories for grpc v1.40.0, libpng12, Swig v4.0.2, OpenCV 4.5.3, Valijson v0.6, CMake v3.21.2, Faiss v1.7.1, and FLINNG. Then download necesarry files for zlib v1.2.13, Json-simple v1.1.1, and TileDB v1.3.1.
Here we assume `$VDMS_DEP_DIR` is the working directory for installing dependencies and `python` is Python 3.
```bash
cd $VDMS_DEP_DIR
git clone --branch v3.21.2 https://github.com/Kitware/CMake.git && \
git clone --branch v4.0.2 https://github.com/swig/swig.git && \
git clone --branch v1.7.1 https://github.com/facebookresearch/faiss.git && \
git clone https://github.com/tonyzhang617/FLINNG.git && \
git clone --branch v1.40.0 https://github.com/grpc/grpc.git && \
git clone --branch 4.5.3 https://github.com/opencv/opencv.git && \
git clone --branch v0.6 https://github.com/tristanpenman/valijson.git

sudo curl -L -o /usr/share/java/json-simple-1.1.1.jar https://storage.googleapis.com/google-code-archive-downloads/v2/code.google.com/json-simple/json-simple-1.1.1.jar && \
sudo curl -L -o 1.3.1.tar.gz https://github.com/TileDB-Inc/TileDB/archive/refs/tags/1.3.1.tar.gz && \
sudo curl -L -o zlib-1.2.13.tar.gz http://zlib.net/zlib-1.2.13.tar.gz
```

### Install Dependencies
These instructions assume you have full permissions to your system.
If running as root, remove `sudo` where necessary.

#### CMAKE
```bash
cd $VDMS_DEP_DIR/CMake && ./bootstrap
make -j && sudo make install
```

### Swig
```bash
cd $VDMS_DEP_DIR/swig
./autogen.sh && ./configure
make -j && sudo make install
```

### Faiss
```bash
cd $VDMS_DEP_DIR/faiss
mkdir build && cd build
cmake -DFAISS_ENABLE_GPU=OFF ..
make -j && sudo make install
```

### FLINNG
```bash
cd $VDMS_DEP_DIR/FLINNG
mkdir build && cd build
cmake ..
make -j && sudo make install
```

### grpc
```bash
cd $VDMS_DEP_DIR/grpc && git submodule update --init --recursive
pip3 install --no-cache-dir -r requirements.txt && \
    GRPC_PYTHON_BUILD_WITH_CYTHON=1 pip3 install --no-cache-dir .

cd tools/distrib/python/grpcio_tools
python ../make_grpcio_tools.py
GRPC_PYTHON_BUILD_WITH_CYTHON=1 pip3 install --no-cache-dir .

cd ../../../../third_party/protobuf/cmake
mkdir build && cd build
cmake -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE ..
make -j && sudo make install

cd ../../../abseil-cpp && mkdir build && cd build
cmake -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE ..
make -j && sudo make install

cd ../../re2/ && mkdir build && cd build
cmake -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE ..
make -j && sudo make install

cd ../../zlib/ && mkdir build && cd build
cmake -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE ..
make -j && sudo make install

cd ../../../cmake && mkdir build && cd build
cmake -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF -DgRPC_ABSL_PROVIDER=package \
    -DgRPC_CARES_PROVIDER=package -DgRPC_PROTOBUF_PROVIDER=package \
    -DgRPC_RE2_PROVIDER=package -DgRPC_SSL_PROVIDER=package \
    -DgRPC_ZLIB_PROVIDER=package -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE ../..
make -j && sudo make install
```

### [OpenCV](https://opencv.org/)

Below are instructions for installing ***OpenCV v4.5.3***. It may also work with newer versions of OpenCV.
```bash
cd $VDMS_DEP_DIR/opencv
mkdir build && cd build
cmake -DBUILD_PERF_TESTS=OFF -DBUILD_TESTS=OFF ..
make -j
sudo make install
```

**Note**: When using videos, and getting the following error: "Unable to stop the stream: Inappropriate ioctl for device", you may need to include more flags when compiling OpenCV. Follow these instructions ([source](https://stackoverflow.com/questions/41200201/opencv-unable-to-stop-the-stream-inappropriate-ioctl-for-device)):
```bash
apt-get install ffmpeg
apt-get install libavcodec-dev libavformat-dev libavdevice-dev

cmake -D BUILD_PERF_TESTS=OFF -D BUILD_TESTS=OFF -D CMAKE_BUILD_TYPE=RELEASE -D CMAKE_INSTALL_PREFIX=/usr/local \
    -D WITH_FFMPEG=ON -D WITH_TBB=ON -D WITH_GTK=ON \
    -D WITH_V4L=ON -D WITH_OPENGL=ON -D WITH_CUBLAS=ON \
    -DWITH_QT=OFF -DCUDA_NVCC_FLAGS="-D_FORCE_INLINES" ..
make -j
make install
```

### Zlib
```bash
cd $VDMS_DEP_DIR && tar -xvzf zlib-1.2.13.tar.gz
cd zlib-1.2.13 && ./configure
make -j && sudo make install
```

### [TileDB](https://tiledb.io/)
VDMS works with ***TileDB v1.3.1.***<br>
The directions below will help you install TileDB v1.3.1 from the source.
You can also follow the directions listed
[here](https://docs.tiledb.io/en/latest/installation.html).
```bash
cd $VDMS_DEP_DIR && tar -xvf 1.3.1.tar.gz
cd TileDB-1.3.1 && mkdir build && cd build
../bootstrap --prefix=/usr/local/
make -j && sudo make install-tiledb
```

### gtest
Unfortunately apt doesn't build gtest;
you need to do the following steps to get it to work correctly:
```bash
cd /usr/src/gtest/
sudo cmake .
sudo make -j
sudo mv lib/libgtest* /usr/lib
```

### Maven
```bash
ln -s $VDMS_DEP_DIR/grpc/third_party/protobuf/cmake/build/protoc $VDMS_DEP_DIR/grpc/third_party/protobuf/src/protoc
cd $VDMS_DEP_DIR/grpc/third_party/protobuf/java/core
mvn package
sudo cp "$(ls target/protobuf-java*.jar)" /usr/share/java/protobuf.jar
```

You may need to change proxy setting for Maven if you are behind a proxy like this example.
Add setting.xml file to ~/.m2 folder
```
<proxies>
    <proxy>
    <id>optional</id>
        <!-- <active>true</active> -->
        <protocol>https</protocol>
        <!--<username>proxyuser</username>
        <password>proxypass</password>-->
        <host>prox-address</host>
        <port>proxy-port</port>
        <nonProxyHosts></nonProxyHosts>
    </proxy>
</proxies>
```

### Valijson
This is a headers-only library, no compilation/installation necessary
```bash
cd $VDMS_DEP_DIR/valijson
sudo cp -r include/* /usr/local/include
```

## Install VDMS
This version of VDMS treats PMGD as a submodule so both libraries are compiled at one time. After entering the vdms directory, the command `git submodule update --init --recursive` will pull pmgd into the appropriate directory. Furthermore, Cmake is used to compile all directories.
```bash
git clone https://github.com/IntelLabs/vdms.git
cd vdms && git checkout develop
git submodule update --init --recursive
```

When compiling on a target without Optane persistent memory, use the following:
```bash
mkdir build && cd build
cmake ..
make -j
cp ../config-vdms.json .
```

When compiling on a target with Optane persistent memory, use the command set:
```bash
mkdir build && cd build
cmake -DCMAKE_CXX_FLAGS='-DPM' ..
make -j
```

