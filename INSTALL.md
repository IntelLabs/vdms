# Installation
Here is the detailed process of installation of VDMS dependencies.

## Dependencies
To install VDMS, we must install the necessary dependencies via apt, github, and pip (Python 3.9+).

### Install Debian/Ubuntu Packages
Here we will install the Debian/Ubuntu packages.
```bash
sudo apt-get update
sudo apt-get install -y --no-install-suggests --no-install-recommends \
    apt-transport-https autoconf automake bison build-essential bzip2 ca-certificates \
    curl ed flex g++-9 gcc-9 git gnupg-agent javacc libarchive-tools libatlas-base-dev \
    libavcodec-dev libavformat-dev libboost-all-dev libbz2-dev libc-ares-dev libcurl4-openssl-dev \
    libdc1394-22-dev libgflags-dev libgoogle-glog-dev libgtk-3-dev libgtk2.0-dev \
    libhdf5-dev libjpeg-dev libjsoncpp-dev libleveldb-dev liblmdb-dev \
    liblz4-dev libopenblas-dev libopenmpi-dev libpng-dev librdkafka-dev libsnappy-dev libssl-dev \
    libswscale-dev libtbb-dev libtbb2 libtiff-dev libtiff5-dev libtool libzmq3-dev linux-libc-dev mpich \
    openjdk-11-jdk-headless pkg-config procps python3-dev python3-pip software-properties-common \
    swig unzip uuid-dev
```
Note: Your system may have g++ or gcc version 10+. If this is the case, please use version 9 to build VDMS. Optional method for setting version 9 as default:
```bash
update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 1
update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-9 1
```

#### **Install JPEG package**
Please install the JPEG package based on the OS platform being used:
* ***Debian 10+:*** `sudo apt-get install -y libjpeg62-turbo-dev`
* ***Ubuntu 20.04+:*** `sudo apt-get install -y libjpeg8-dev`
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
Here we will install the necessary Python 3.9+ packages Numpy and Protobuf v24.2.
It is expected that you have Python3.9 or higher installed on your system.
All python calls will use Python3.9+; therefore you may find it convenient to set alias for python.
```bash
alias python=/usr/bin/python3
```
***NOTE:*** If multiple versions of Python 3 are present on your system, verify you are using Python3.9 or higher. You can specify the specific verison in above command and also set the following with your specific version: `alias python3=/usr/bin/python3.x`.

You can also install the coverage package if interested in running the Python unit tests.
```bash
python3 -m pip install --upgrade pip
python3 -m pip install --no-cache-dir "numpy>=1.25.1" "coverage>=7.2.7"
```


#### **CMAKE v3.27.2**
VDMS requires CMake v3.21+.  Here we install CMake v3.27.2.
```bash
CMAKE_VERSION="v3.27.2"
git clone --branch ${CMAKE_VERSION} https://github.com/Kitware/CMake.git $VDMS_DEP_DIR/CMake
cd $VDMS_DEP_DIR/CMake
./bootstrap
make ${BUILD_THREADS}
sudo make install
```


#### **Faiss v1.7.3**
Install the Faiss library for similarity search.
```bash
FAISS_VERSION="v1.7.3"
git clone --branch ${FAISS_VERSION} https://github.com/facebookresearch/faiss.git $VDMS_DEP_DIR/faiss
cd $VDMS_DEP_DIR/faiss
mkdir build && cd build
cmake -DFAISS_ENABLE_GPU=OFF -DPython_EXECUTABLE=/usr/bin/python3 ..
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


#### **Protobuf v24.2 (4.24.2)**
Install Protobuf (C++ and Python) which requires GoogleTest and Abseil C++ as dependencies.
```bash
PROTOBUF_VERSION="24.2"
git clone -b v${PROTOBUF_VERSION} --recursive https://github.com/protocolbuffers/protobuf.git $VDMS_DEP_DIR/protobuf

cd $VDMS_DEP_DIR/protobuf/third_party/googletest
mkdir build && cd build
cmake -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DBUILD_GMOCK=ON -DCMAKE_CXX_STANDARD=17 ..
make ${BUILD_THREADS}
sudo make install
sudo ldconfig

cd $VDMS_DEP_DIR/protobuf/third_party/abseil-cpp
mkdir build && cd build
cmake -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_PREFIX_PATH=/usr/local/ -DCMAKE_INSTALL_PREFIX=/usr/local/ \
    -DABSL_BUILD_TESTING=ON -DABSL_ENABLE_INSTALL=ON -DABSL_USE_EXTERNAL_GOOGLETEST=ON \
    -DABSL_FIND_GOOGLETEST=ON -DCMAKE_CXX_STANDARD=17 ..
make ${BUILD_THREADS}
sudo make install

cd $VDMS_DEP_DIR/protobuf
cmake -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_CXX_STANDARD=17 \
    -Dprotobuf_ABSL_PROVIDER=package -DCMAKE_PREFIX_PATH=/usr/local .
make ${BUILD_THREADS}
sudo make install

python3 -m pip install --no-cache-dir "protobuf==4.${PROTOBUF_VERSION}"
```


#### **[OpenCV](https://opencv.org/) 4.5.5**
Below are instructions for installing ***OpenCV v4.5.5***.
```bash
OPENCV_VERSION="4.5.5"
git clone --branch ${OPENCV_VERSION} https://github.com/opencv/opencv.git $VDMS_DEP_DIR/opencv
cd $VDMS_DEP_DIR/opencv
mkdir build && cd build
cmake -D BUILD_PERF_TESTS=OFF -D BUILD_TESTS=OFF ..
make ${BUILD_THREADS}
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


#### **AWS SDK CPP 1.11.0**
Use the following instructions to install AWS SDK for C++.
```bash
AWS_SDK_VERSION="1.11.0"
git clone -b ${AWS_SDK_VERSION} --recurse-submodules https://github.com/aws/aws-sdk-cpp ${VDMS_DEP_DIR}/aws-sdk-cpp
mkdir -p ${VDMS_DEP_DIR}/aws-sdk-cpp/build
cd ${VDMS_DEP_DIR}/aws-sdk-cpp/build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/usr/local/ -DCMAKE_INSTALL_PREFIX=/usr/local/ -DBUILD_ONLY="s3" -DCUSTOM_MEMORY_MANAGEMENT=OFF
make ${BUILD_THREADS}
sudo make install
```
<br>

## Install VDMS
This version of VDMS treats PMGD as a submodule so both libraries are compiled at one time. After entering the vdms directory, the command `git submodule update --init --recursive` will pull pmgd into the appropriate directory. Furthermore, Cmake is used to compile all directories.
```bash
git clone -b develop https://github.com/IntelLabs/vdms.git
cd vdms
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

