# Pull base image.
FROM ubuntu:16.04

RUN apt-get update && apt-get upgrade -y && \
    apt-get install -y \
        build-essential scons autoconf automake libtool curl make g++ unzip \
        bzip2 cmake git pkg-config \
        python python-dev python-pip python-virtualenv \
        wget zlib1g-dev libbz2-dev libssl-dev liblz4-dev mpich \
        libjsoncpp-dev flex javacc libbison-dev openjdk-8-jdk \
        libleveldb-dev libsnappy-dev libhdf5-serial-dev \
        libatlas-base-dev libboost-all-dev libgflags-dev libgoogle-glog-dev \
        liblmdb-dev \
        libjpeg8-dev libtiff5-dev libjasper-dev libpng12-dev libgtk-3-dev \
        libopenmpi-dev \
        libgtest-dev ed \
        libgtk2.0-dev pkg-config libavcodec-dev \
        libavformat-dev libswscale-dev \
        libtbb2 libtbb-dev libjpeg-dev libpng-dev \
        libtiff-dev libdc1394-22-dev libopenblas-dev

RUN apt-get remove -y libprotobuf-dev protobuf-compiler

# Google Test
RUN cd /usr/src/gtest && cmake . && make && mv libgtest* /usr/lib/

# Protobuf
RUN cd / && git clone https://github.com/google/protobuf.git && \
    cd protobuf && git checkout v3.6.1 && ./autogen.sh &&  \
    ./configure --prefix=/usr/local && \
    make -j $(cat /proc/cpuinfo | wc -l) && \
    make install && ldconfig
RUN rm -rf /protobuf

# OpenCV
RUN mkdir /opencv && cd /opencv && \
    wget -O opencv.zip https://github.com/opencv/opencv/archive/3.3.0.zip && \
    unzip opencv.zip && cd opencv-3.3.0 && \
    mkdir build && cd build && \
    cmake -D CMAKE_BUILD_TYPE=Release \
    -D CMAKE_INSTALL_PREFIX=/usr/local .. && \
    make -j $(cat /proc/cpuinfo | wc -l) && make install
RUN rm -rf /opencv

# Blosc
RUN cd / && git clone https://github.com/Blosc/c-blosc.git && \
    cd c-blosc && mkdir build && cd build && \
    cmake -DCMAKE_INSTALL_PREFIX='/usr' .. && cmake --build . && \
    cmake --build . --target install
RUN rm -rf /c-blosc

# Zstd
RUN cd / && wget https://github.com/facebook/zstd/archive/v1.1.0.tar.gz && \
    tar xf v1.1.0.tar.gz && cd zstd-1.1.0 && \
    make install PREFIX='/usr'

# TileDB
RUN cd / && wget https://github.com/TileDB-Inc/TileDB/archive/1.3.1.tar.gz && \
    tar xf 1.3.1.tar.gz && cd TileDB-1.3.1 && mkdir build && cd build && \
    ../bootstrap --prefix=/usr/local/ && make -j && make install-tiledb
RUN rm -rf /TileDB-1.3.1

# Faiss
RUN cd / && \
    wget https://github.com/facebookresearch/faiss/archive/v1.4.0.tar.gz && \
    tar xf v1.4.0.tar.gz && \
    cd faiss-1.4.0 && rm CMakeLists.txt && \
    wget https://gist.githubusercontent.com/luisremis/758c71c6a86cb3e4fe2311c415e07547/raw/3c51b5ed7d3f16a4d684d9ea524602c9f1f8593a/CMakeLists.txt && \
    mkdir build && cd build && cmake ../ && make -j
RUN mkdir /usr/local/include/faiss/
RUN cp -r faiss-1.4.0/* /usr/local/include/faiss/
RUN mv faiss-1.4.0/build/lib/libfaiss.so /usr/local/lib/
RUN rm -rf faiss-1.4*

# PMGD install
RUN cd / && wget https://github.com/intellabs/pmgd/archive/v2.0.0.tar.gz && \
    tar xf v2.0.0.tar.gz && rm v2.0.0.tar.gz && mv pmgd-2.0.0 pmgd && \
    cd pmgd && make PMOPT=MSYNC

# VCL install
RUN cd / && wget https://github.com/intellabs/vcl/archive/v1.0.0.tar.gz && \
    tar xf v1.0.0.tar.gz && rm v1.0.0.tar.gz && mv vcl-1.0.0 vcl && \
    cd vcl && scons -j16

# Valijson
RUN cd / && git clone https://github.com/tristanpenman/valijson.git && \
    cd valijson && cp -r include/* /usr/local/include/

# VDMS install
RUN wget https://github.com/intellabs/vdms/archive/v2.0.0.tar.gz && \
    tar xf v2.0.0.tar.gz && rm v2.0.0.tar.gz && mv vdms-2.0.0 vdms && \
    cd vdms && scons -j16 INTEL_PATH=/

RUN rm -rf /*.tar.gz
RUN rm -rf /var/lib/apt/lists/* /root/.cache

# Setup entry point

RUN echo '#!/bin/bash' > /start.sh
RUN echo 'export LD_LIBRARY_PATH=/vdms/utils:/pmgd/lib:$(find /usr/local/lib/ / -type f -name "*.so" | xargs dirname | sort | uniq | tr "\n" ":")' >> /start.sh
RUN echo 'cd /vdms' >> /start.sh
RUN echo 'rm -rf db' >> /start.sh
RUN echo './vdms' >> /start.sh
RUN chmod 755 /start.sh

EXPOSE 55555
CMD ["/start.sh"]
