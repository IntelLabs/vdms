## Installation

Here is the detailed process of installation of VDMS dependencies.

## Dependencies
    sudo apt-get update && \\

    sudo apt-get -y install software-properties-common && \\

    add-apt-repository "deb http://security.ubuntu.com/ubuntu xenial-security main" && \\

    sudo apt-get -y install g++ git libssl-dev libc-ares-dev apt-transport-https ca- 
    certificates curl && \\

    sudo apt-get -y installgnupg-agent software-properties-common cmake python3-pip build- 
    essential autoconf automaker && \\

    sudo apt-get -y install libtool g++ unzip bzip2 libarchive-tools cmake git pkg-config 
    python python-dev wget libbz2- dev libssl-dev liblz4-dev mpich libjsoncpp-dev flex javacc 
    bison && \\

    sudo apt-get -y install openjdk-11-jdk-headless libleveldb-dev libsnappy-dev libhdf5- 
    serial-dev libatlas-base-dev libboost-all-dev libgflags-dev libgoogle-glog-dev 
    liblmdb-dev  && \\

    sudo apt-get -y install libjpeg8-dev libtiff5-dev libjasper-dev libgtk-3-dev 
    libopenmpi-dev libgtest-dev ed libgtk2.0-dev pkg-config libavcodec-dev libavformat-dev 
    libswscale-dev && \\

    sudo apt-get -y install libtbb2 libtbb-dev libjpeg-dev libpng-dev libtiff-dev 
    libdc1394-22-dev libopenblas-dev && \\

    sudo pip3 install numpy
### Pull Dependencies
     git clone https://github.com/grpc/grpc.git && \\
     git clone https://github.com/glennrp/libpng.git && \\
     git clone https://github.com/swig/swig.git && \\
     git clone https://github.com/opencv/opencv.git && \\
     git clone https://github.com/tristanpenman/valijson.git && \\
     git clone https://github.com/Kitware/CMake.git && \\
     git clone https://github.com/facebookresearch/faiss.git && \\
     curl http://zlib.net/zlib-1.2.11.tar.gz -o zlib-1.2.11.tar.gz

### Install CMAKE 
    cd CMake && ./bootstrap && make -j16 && make install && \\
    cd / && cd swig && \ 
    ./autogen.sh && ./configure && make -j16 && make install 
### Install Faiss
    cd faiss && mkdir build && cd build && \
    cmake -DFAISS_ENABLE_GPU=OFF .. && \\
    make -j16 && make install

### Install grpc
    cd grpc && git submodule update --init --recursive && \
    cd third_party/protobuf/cmake && mkdir build && cd build && cmake - 
    DCMAKE_POSITION_INDEPENDENT_CODE=TRUE .. && \
    make -j8 && make install && cd ../../../abseil-cpp && mkdir build && cd build && \
    cmake -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE .. && make -j8 && make install && cd 
    ../../re2/ && mkdir build && \
    cd build && cmake -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE .. && make -j8 && make install 
     && cd ../../zlib/ && \
    mkdir build && cd build && cmake -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE .. && make -j8 && 
    make install && \
    cd /grpc/cmake && mkdir build && cd build && cmake -DgRPC_INSTALL=ON                     
     -DRPC_BUILD_TESTS=OFF -DgRPC_ABSL_PROVIDER=package \
    -DgRPC_CARES_PROVIDER=package -DgRPC_PROTOBUF_PROVIDER=package DgRPC_RE2_PROVIDER=package 
    -DgRPC_SSL_PROVIDER=package \
    -DgRPC_ZLIB_PROVIDER=package -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE ../.. && make -j8 && 
     make install

### Install gtest

Unfortunately apt doesn't build gtest;
you need to do the following steps to get it to work correctly:

    cd /usr/src/gtest/
    sudo cmake CMakeLists.txt
    sudo make
    sudo cp *.a /usr/lib

### Install Zlib
    gunzip zlib-1.2.11.tar.gz && \\
    tar -xvf zlib-1.2.11.tar && \\
    cd zlib-1.2.11 && ./configure && make -j4 && \
    sudo make install
### Install Libpng
    cd libpng && \\
    git checkout libpng12 && ./configure &&\\
    make -j4 && make install


### Protobuf

 Google Protobufs. Default install location is /usr/local.

    git clone https://github.com/google/protobuf.git
    cd protobuf/
    ./autogen.sh
    ./configure
    make && make check
    sudo make install
    sudo ldconfg

**Note**: protobuf v3.9+ is required
(previous versions cause problems in some systems)

### [OpenCV](https://opencv.org/)

Below are instructions for installing OpenCV v3.3.1.
It may also work with newer versions of OpenCV.

    wget https://github.com/opencv/opencv/archive/3.3.1.zip
    unzip 3.3.1.zip
    cd 3.3.1
    mkdir build
    cd build/
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local ..
    make -j
    sudo make install

**Note**: When using videos, and getting the following error: "Unable to stop the stream: Inappropriate ioctl for device", you may need to include more flags when compiling OpenCV. Follow these instructions ([source](https://stackoverflow.com/questions/41200201/opencv-unable-to-stop-the-stream-inappropriate-ioctl-for-device)):

    sudo apt-get install ffmpeg
    sudo apt-get install libavcodec-dev libavformat-dev libavdevice-dev

    // Rebuild OpenCV with the following commands:

    cmake -D CMAKE_BUILD_TYPE=RELEASE -D CMAKE_INSTALL_PREFIX=/usr/local  -D WITH_FFMPEG=ON -D WITH_TBB=ON -D WITH_GTK=ON -D WITH_V4L=ON -D WITH_OPENGL=ON -D WITH_CUBLAS=ON -DWITH_QT=OFF -DCUDA_NVCC_FLAGS="-D_FORCE_INLINES" ..
    make -j
    sudo make install

### [TileDB](https://tiledb.io/)
VDMS works with TileDB v1.3.1.
The directions below will help you install TileDB v1.3.1 from the source.
You can also follow the directions listed
[here](https://docs.tiledb.io/en/latest/installation.html).

Build TileDB

    git clone https://github.com/TileDB-Inc/TileDB
    cd TileDB
    git checkout 1.3.1
    mkdir build
    cd build
    ../bootstrap --prefix=/usr/local/
    make -j
    sudo make install-tiledb

### [Maven]
    sudo ln -s /grpc/third_party/protobuf/cmake/build/protoc 
    grpc/third_party/protobuf/src/protoc && \
    sudo apt-get install -y maven && \
    cd /grpc/third_party/protobuf/java/core &&\
    mvn package &&\
    cp target/protobuf-java-3.13.0.jar /usr/share/java/protobuf.jar &&\
    curl https://storage.googleapis.com/google-code-archive- 
    downloads/v2/code.google.com/json-simple/json-simple-1.1.1.jar -o /usr/share/java/json- 
    simple-1.1.1.jar && cd / &&\
    curl https://mirror.jframeworks.com/apache//commons/codec/binaries/commons-codec-1.15- 
    bin.tar.gz -o /commons-codec-1.15-bin.tar.gz &&\ 
    gunzip /commons-codec-1.15-bin.tar.gz &&\
    tar -xvf commons-codec-1.15-bin.tar && \
    cp commons-codec-1.15/commons-codec-1.15.jar /usr/share/java/commons-codec-1.15.jar

You may need to change proxy setting for Maven if you are behind a proxy like this example:

### Adding the setting.xml file to ~/.m2 folder
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

### Valijson
  * git clone https://github.com/tristanpenman/valijson.git
  * cd valijson
  * cp -r include/* /usr/local/include (may need to run as sudo)
  * This is a headers-only library, no compilation/installation necessary

### Install VDMS
    git clone https://github.com/IntelLabs/vdms.git &&\
    cd vdms && git checkout develop &&\
    git submodule update --init --recursive && \
    cd vdms && mkdir build && cd build && cmake .. && make -j16 && \
    cp ../config-vdms.json . 



## Compilation
This version of VDMS treats PMGD as a submodule so both libraries are compiled at one time. After entering the vdms directory, the command "git submodule update --init --recursive" will pull pmgd into the appropriate directory. Furthermore, Cmake is used to compile all directories. When compiling on a target with Optane persistent memory, use the command set

mkdir build && cd build && cmake -DCMAKE_CXX_FLAGS='-DPM' .. && make -j<<number of threads to use for compiling>>

For systems without Optane, use the command set
mkdir build && cd build && cmake .. && make -j<<number of threads to use for compiling>>
