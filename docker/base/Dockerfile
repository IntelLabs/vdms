#Copyright (C) 2021 Intel Corporation
#SPDX-License-Identifier: MIT

ARG UBUNTU_VERSION=20.04
ARG UBUNTU_NAME=focal
ARG BUILD_THREADS=-j16
ARG MAVEN_OPTS='-Dhttps.nonProxyHosts="localhost|127.0.0.1"'

#1
FROM ubuntu:${UBUNTU_VERSION}

# Dockerfile limitations force a repetition of global args
ARG UBUNTU_VERSION
ARG UBUNTU_NAME
ARG MAVEN_OPTS

# Install Packages
RUN apt-get update && apt-get -y install software-properties-common && \
    add-apt-repository "deb http://security.ubuntu.com/ubuntu ${UBUNTU_NAME}-security main" && \
    apt-get -y install apt-transport-https autoconf automake bison build-essential \
    bzip2 ca-certificates curl ed flex g++ git gnupg-agent javacc libarchive-tools \
    libatlas-base-dev libavcodec-dev libavformat-dev libboost-all-dev libbz2-dev \
    libc-ares-dev libdc1394-22-dev libgflags-dev libgoogle-glog-dev libgtest-dev \
    libgtk-3-dev libgtk2.0-dev libhdf5-serial-dev libjpeg-dev libjpeg8-dev libjsoncpp-dev \
    libleveldb-dev liblmdb-dev liblz4-dev libopenblas-dev libopenmpi-dev \
    libpng-dev librdkafka-dev libsnappy-dev libssl-dev libswscale-dev libtbb-dev \
    libtbb2 libtiff-dev libtiff5-dev libtool maven mpich openjdk-11-jdk-headless \
    pkg-config python python-dev python3-pip unzip wget

RUN pip3 install numpy

# Pull Dependencies
RUN git clone --branch v1.40.0 https://github.com/grpc/grpc.git && \
    git clone --branch v4.0.2 https://github.com/swig/swig.git && \
    git clone --branch 4.5.3 https://github.com/opencv/opencv.git && \
    git clone --branch v0.6 https://github.com/tristanpenman/valijson.git && \
    git clone --branch v3.21.2 https://github.com/Kitware/CMake.git && \
    git clone --branch v1.7.1 https://github.com/facebookresearch/faiss.git && \
    git clone https://github.com/tonyzhang617/FLINNG.git && \
    curl http://zlib.net/zlib-1.2.12.tar.gz -o zlib-1.2.12.tar.gz && \
    curl https://storage.googleapis.com/google-code-archive-downloads/v2/code.google.com/json-simple/json-simple-1.1.1.jar -o /usr/share/java/json-simple-1.1.1.jar && \
    wget https://github.com/TileDB-Inc/TileDB/archive/1.3.1.tar.gz

# Install Dependencies
RUN cd /CMake && ./bootstrap && make ${BUILD_THREADS} && make install && \
    cd /swig && ./autogen.sh && ./configure && make ${BUILD_THREADS} && make install && \
    cd /faiss && mkdir build && cd build && cmake -DFAISS_ENABLE_GPU=OFF .. && make ${BUILD_THREADS} && make install && \
    cd /FLINNG && mkdir build && cd build && cmake .. && make ${BUILD_THREADS} && make install && \
    cd /grpc && git submodule update --init --recursive && cd third_party/protobuf/cmake && mkdir build && cd build && cmake -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE .. && make ${BUILD_THREADS} && make install && \
    cd ../../../abseil-cpp && mkdir build && cd build && cmake -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE .. && make ${BUILD_THREADS} && make install && \
    cd ../../re2/ && mkdir build && cd build && cmake -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE .. && make ${BUILD_THREADS} && make install && \
    cd ../../zlib/ && mkdir build && cd build && cmake -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE .. && make ${BUILD_THREADS} && make install && \
    cd /grpc/cmake && mkdir build && cd build && cmake -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF -DgRPC_ABSL_PROVIDER=package \
    -DgRPC_CARES_PROVIDER=package -DgRPC_PROTOBUF_PROVIDER=package \
    -DgRPC_RE2_PROVIDER=package -DgRPC_SSL_PROVIDER=package \
    -DgRPC_ZLIB_PROVIDER=package -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE ../.. && make ${BUILD_THREADS} && make install && \
    cd / && gunzip zlib-1.2.12.tar.gz && tar -xvf zlib-1.2.12.tar && cd zlib-1.2.12 && ./configure && make ${BUILD_THREADS} && make install && \
    cd / && rm -rf zlib-1.2.12.tar zlib-1.2.12

# Google Test & OpenCV
RUN cd /usr/src/gtest && cmake . && make ${BUILD_THREADS} && mv lib/libgtest* /usr/lib/ && \
    cd /opencv && mkdir build && cd build && cmake -D BUILD_PERF_TESTS=OFF -D BUILD_TESTS=OFF .. && make ${BUILD_THREADS} && make install

# TileDB
RUN cd / && tar xf 1.3.1.tar.gz && rm 1.3.1.tar.gz && \
    cd TileDB-1.3.1 && mkdir build && cd build && \
    ../bootstrap --prefix=/usr/local/ && make $BUILD_THREADS && make install-tiledb && \
    rm -rf /TileDB-1.3.1

# Maven
RUN ln -s /grpc/third_party/protobuf/cmake/build/protoc /grpc/third_party/protobuf/src/protoc && \
    cd /grpc/third_party/protobuf/java/core && mvn package && \
    cp $(ls target/protobuf-java*.jar) /usr/share/java/protobuf.jar

# Valijson
RUN cd /valijson && cp -r include/* /usr/local/include/ && \
    cd / && rm -rf valijson && rm -rf faiss && \
    rm -rf grpc && rm -rf opencv && rm -rf swig && rm -rf CMake

# VDMS
RUN git clone https://github.com/IntelLabs/vdms.git && cd vdms && \
    git checkout develop && git submodule update --init --recursive && \
    mkdir build && cd build && cmake .. && make ${BUILD_THREADS} && \
    cp /vdms/config-vdms.json /vdms/build/

RUN echo '#!/bin/bash' > /start.sh && echo 'cd /vdms/build' >> /start.sh && \
    echo './vdms' >> /start.sh && chmod 755 /start.sh

CMD ["/start.sh"]
