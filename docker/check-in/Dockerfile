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

#Install Packages
RUN apt-get update && apt-get -y install software-properties-common && \
    add-apt-repository "deb http://security.ubuntu.com/ubuntu xenial-security main" && \
    apt-get -y install g++ git libssl-dev libc-ares-dev apt-transport-https \
    ca-certificates curl gnupg-agent software-properties-common cmake python3-pip \
    build-essential autoconf automake libtool g++ unzip bzip2 libarchive-tools \
    cmake git pkg-config python python-dev wget libbz2-dev libssl-dev liblz4-dev \
    mpich libjsoncpp-dev flex javacc bison openjdk-11-jdk-headless libleveldb-dev \
    libsnappy-dev libhdf5-serial-dev libatlas-base-dev libboost-all-dev \
    libgflags-dev libgoogle-glog-dev liblmdb-dev libjpeg8-dev libtiff5-dev \
    libjasper-dev libgtk-3-dev libopenmpi-dev libgtest-dev ed libgtk2.0-dev \
    pkg-config libavcodec-dev libavformat-dev libswscale-dev libtbb2 libtbb-dev \
    libjpeg-dev libpng-dev libtiff-dev libdc1394-22-dev libopenblas-dev maven lcov && \
    pip3 install numpy coverage vdms gcovr

#Pull Dependencies
RUN git clone --branch v1.40.0 https://github.com/grpc/grpc.git && \
    git clone --branch libpng12 https://github.com/glennrp/libpng.git && \
    git clone --branch v4.0.2 https://github.com/swig/swig.git && \
    git clone --branch 4.5.3 https://github.com/opencv/opencv.git && \
    git clone --branch v0.6 https://github.com/tristanpenman/valijson.git && \
    git clone --branch v3.21.2 https://github.com/Kitware/CMake.git && \
    git clone --branch v1.7.1 https://github.com/facebookresearch/faiss.git && \
    curl http://zlib.net/zlib-1.2.11.tar.gz -o zlib-1.2.11.tar.gz && \
    curl https://downloads.apache.org/commons/codec/binaries/commons-codec-1.15-bin.tar.gz -o /commons-codec-1.15-bin.tar.gz && \
    curl https://storage.googleapis.com/google-code-archive-downloads/v2/code.google.com/json-simple/json-simple-1.1.1.jar -o /usr/share/java/json-simple-1.1.1.jar && \
    wget https://github.com/TileDB-Inc/TileDB/archive/1.3.1.tar.gz

RUN cd /CMake && ./bootstrap && make ${BUILD_THREADS} && make install && \
    cd /swig && ./autogen.sh && ./configure && make ${BUILD_THREADS} && make install && \
    cd /faiss && mkdir build && cd build && cmake -DFAISS_ENABLE_GPU=OFF .. && make ${BUILD_THREADS} && make install && \
    cd /grpc && git submodule update --init --recursive && cd third_party/protobuf/cmake && mkdir build && cd build && cmake -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE .. && make -j8 && make install && \
    cd ../../../abseil-cpp && mkdir build && cd build && cmake -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE .. && make -j8 && make install && \
    cd ../../re2/ && mkdir build && cd build && cmake -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE .. && make -j8 && make install && \
    cd ../../zlib/ && mkdir build && cd build && cmake -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE .. && make -j8 && make install && \
    cd /grpc/cmake && mkdir build && cd build && cmake -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF -DgRPC_ABSL_PROVIDER=package \
    -DgRPC_CARES_PROVIDER=package -DgRPC_PROTOBUF_PROVIDER=package \
    -DgRPC_RE2_PROVIDER=package -DgRPC_SSL_PROVIDER=package \
    -DgRPC_ZLIB_PROVIDER=package -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE ../.. && make -j8 && make install && \
    cd / && gunzip zlib-1.2.11.tar.gz && tar -xvf zlib-1.2.11.tar && cd zlib-1.2.11 && ./configure && make -j4 && make install && \
    cd / && rm -rf zlib-1.2.11.tar zlib-1.2.11 && \
    cd /libpng && ./configure && make -j4 && make install

# Google Test
RUN cd /usr/src/gtest && cmake . && make ${BUILD_THREADS} && mv lib/libgtest* /usr/lib/ && \
    cd /opencv && mkdir build && cd build && cmake -D BUILD_PERF_TESTS=OFF -D BUILD_TESTS=OFF .. && make ${BUILD_THREADS} && make install

# TileDB
RUN cd / && tar xf 1.3.1.tar.gz && rm 1.3.1.tar.gz && \
    cd TileDB-1.3.1 && mkdir build && cd build && \
    ../bootstrap --prefix=/usr/local/ && make $BUILD_THREADS && make install-tiledb && \
    rm -rf /TileDB-1.3.1

RUN ln -s /grpc/third_party/protobuf/cmake/build/protoc /grpc/third_party/protobuf/src/protoc && \
    cd /grpc/third_party/protobuf/java/core && mvn package && \
    cp $(ls target/protobuf-java*.jar) /usr/share/java/protobuf.jar

# Valijson
RUN cd /valijson && cp -r include/* /usr/local/include/

COPY . /vdms
RUN cd /vdms && git submodule update --init --recursive && mkdir build && \
    cd build && cmake -DCODE_COVERAGE=ON .. && make ${BUILD_THREADS} && cd / && rm -rf valijson && rm -rf faiss && \
    rm -rf grpc && rm -rf libpng  && rm -rf opencv && rm -rf swig && rm -rf CMake && \
    cp /vdms/config-vdms.json /vdms/build/

RUN echo '#!/bin/bash' > /start.sh && echo 'cd /vdms/build' >> /start.sh && \
    echo './vdms' >> /start.sh && chmod 755 /start.sh

CMD ["/start.sh"]
