# Pull base image.
FROM ubuntu:16.04

RUN apt-get update && apt-get upgrade -y && \
    apt-get install -y --no-install-recommends \
        build-essential scons autoconf automake libtool curl make g++ unzip \
        bzip2 bsdtar cmake git pkg-config \
        python python-dev python-pip python-virtualenv \
        wget zlib1g-dev libbz2-dev libssl-dev liblz4-dev mpich \
        libjsoncpp-dev flex javacc bison openjdk-8-jdk \
        libleveldb-dev libsnappy-dev libhdf5-serial-dev \
        libatlas-base-dev libboost-all-dev libgflags-dev libgoogle-glog-dev \
        liblmdb-dev \
        libjpeg8-dev libtiff5-dev libjasper-dev libpng12-dev libgtk-3-dev \
        libopenmpi-dev \
        libgtest-dev ed \
        libgtk2.0-dev pkg-config libavcodec-dev \
        libavformat-dev libswscale-dev \
        libtbb2 libtbb-dev libjpeg-dev libpng-dev \
        libtiff-dev libdc1394-22-dev libopenblas-dev && \
        apt-get remove -y libprotobuf-dev protobuf-compiler && \
        rm -rf /var/lib/apt/lists/* /root/.cache

# Google Test
RUN cd /usr/src/gtest && cmake . && make && mv libgtest* /usr/lib/

# Protobuf
RUN git clone https://github.com/google/protobuf.git && \
    cd protobuf && git checkout v3.9.0 && ./autogen.sh &&  \
    ./configure --prefix=/usr/local && \
    make -j $(cat /proc/cpuinfo | wc -l) && \
    make install && ldconfig && \
    rm -rf /protobuf

# OpenCV
RUN mkdir /opencv && cd /opencv && \
    wget -O opencv.zip https://github.com/opencv/opencv/archive/3.3.0.zip && \
    unzip opencv.zip && rm opencv.zip && cd opencv-3.3.0 && \
    mkdir build && cd build && \
    cmake -D CMAKE_BUILD_TYPE=Release \
    -D CMAKE_INSTALL_PREFIX=/usr/local .. && \
    make -j $(cat /proc/cpuinfo | wc -l) && make install && \
    rm -rf /opencv

# TileDB
RUN wget https://github.com/TileDB-Inc/TileDB/archive/1.3.1.tar.gz && \
    tar xf 1.3.1.tar.gz && rm 1.3.1.tar.gz && \
    cd TileDB-1.3.1 && mkdir build && cd build && \
    ../bootstrap --prefix=/usr/local/ && make -j && make install-tiledb && \
    rm -rf /TileDB-1.3.1

# Faiss
RUN wget https://github.com/facebookresearch/faiss/archive/v1.4.0.tar.gz && \
    tar xf v1.4.0.tar.gz && rm v1.4.0.tar.gz && \
    cd faiss-1.4.0 && rm CMakeLists.txt && \
    wget https://gist.githubusercontent.com/luisremis/758c71c6a86cb3e4fe2311c415e07547/raw/3c51b5ed7d3f16a4d684d9ea524602c9f1f8593a/CMakeLists.txt && \
    mkdir build && cd build && cmake ../ && make -j && \
    mkdir /usr/local/include/faiss/ && \
    cp -r /faiss-1.4.0/* /usr/local/include/faiss/ && \
    mv /faiss-1.4.0/build/lib/libfaiss.so /usr/local/lib/ && \
    rm -rf /faiss-1.4*

# Valijson
RUN git clone https://github.com/tristanpenman/valijson.git && \
    cd valijson && cp -r include/* /usr/local/include/ && \
    rm -r /valijson

# VDMS/PMGD install
RUN git clone https://github.com/intellabs/vdms/ vdms_install && \
    cd vdms_install && git checkout develop && \
    git clone https://github.com/intellabs/pmgd/ && \
    cd pmgd && git checkout develop && \
    make PMOPT=MSYNC && \
    mv lib/*.so /usr/local/lib/ && ls /usr/local/lib/libpmgd* && \
    cd .. && scons -j16 install && \
    cd / && mkdir vdms && cp vdms_install/config-vdms.json vdms/ && \
    rm -rf vdms_install pmgd && ldconfig
