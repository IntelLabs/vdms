#Copyright (C) 2021 Intel Corporation
#SPDX-License-Identifier: MIT

ARG UBUNTU_VERSION=20.04
ARG UBUNTU_NAME=focal
ARG BUILD_THREADS=-j16

#1
FROM ubuntu:${UBUNTU_VERSION}

# Dockerfile limitations force a repetition of global args
ARG UBUNTU_VERSION
ARG UBUNTU_NAME

# Install Packages
RUN apt-get update && apt-get install -y --no-install-recommends software-properties-common && \
    add-apt-repository "deb http://security.ubuntu.com/ubuntu ${UBUNTU_NAME}-security main" && \
    apt-get install -y --no-install-recommends apt-transport-https autoconf automake bison build-essential \
    bzip2 ca-certificates curl=7.68.0-1ubuntu2.18 ed flex g++ git gnupg-agent javacc libarchive-tools \
    libatlas-base-dev libavcodec-dev libavformat-dev libboost-all-dev libbz2-dev \
    libc-ares-dev libdc1394-22-dev libgflags-dev libgoogle-glog-dev libgtest-dev \
    libgtk-3-dev libgtk2.0-dev libhdf5-serial-dev libjpeg-dev libjpeg8-dev libjsoncpp-dev \
    libleveldb-dev liblmdb-dev liblz4-dev libopenblas-dev libopenmpi-dev \
    libpng-dev librdkafka-dev libsnappy-dev libssl-dev libswscale-dev libtbb-dev \
    libtbb2 libtiff-dev libtiff5-dev libtool mpich openjdk-11-jdk-headless \
    libcurl4-openssl-dev uuid-dev zlib1g-dev libpulse-dev libzmq3-dev \
    pkg-config python3-dev python3-pip unzip && \
    apt-get clean && rm -rf /var/lib/apt/lists/* && \
    update-alternatives --install /usr/bin/python python /usr/bin/python3 1 && \
    pip3 install --no-cache-dir "numpy>=1.23.2" "setuptools>=65.5.1"

# Pull and Install Dependencies
WORKDIR /dependencies
RUN git clone --branch v3.21.2 https://github.com/Kitware/CMake.git && \
    git clone --branch v4.0.2 https://github.com/swig/swig.git && \
    git clone --branch v1.7.1 https://github.com/facebookresearch/faiss.git && \
    git clone https://github.com/tonyzhang617/FLINNG.git && \
    git clone --recurse-submodules -b v1.40.0 https://github.com/grpc/grpc.git && \
    git clone --branch 4.5.3 https://github.com/opencv/opencv.git && \
    git clone --branch v0.6 https://github.com/tristanpenman/valijson.git && \
    git clone --recurse-submodules https://github.com/aws/aws-sdk-cpp && \
    curl -L -o /usr/share/java/json-simple-1.1.1.jar https://storage.googleapis.com/google-code-archive-downloads/v2/code.google.com/json-simple/json-simple-1.1.1.jar && \
    curl -L -o /dependencies/2.14.0.tar.gz https://github.com/TileDB-Inc/TileDB/archive/refs/tags/2.14.0.tar.gz && \
    curl -L -o /dependencies/zlib-1.2.13.tar.gz http://zlib.net/zlib-1.2.13.tar.gz && \
    cd /dependencies/CMake && ./bootstrap && make ${BUILD_THREADS} && make install && \
    cd /dependencies/swig && ./autogen.sh && ./configure && make ${BUILD_THREADS} && make install && \
    cd /dependencies/faiss && mkdir build && cd build && cmake -DFAISS_ENABLE_GPU=OFF .. && make ${BUILD_THREADS} && make install && \
    cd /dependencies/FLINNG && mkdir build && cd build && cmake .. && make ${BUILD_THREADS} && make install && \
    cd /dependencies/grpc && pip3 install --no-cache-dir -r requirements.txt && GRPC_PYTHON_BUILD_WITH_CYTHON=1 pip3 install --no-cache-dir . && \
        cd tools/distrib/python/grpcio_tools && python ../make_grpcio_tools.py && GRPC_PYTHON_BUILD_WITH_CYTHON=1 pip3 install --no-cache-dir . && \
        cd /dependencies/grpc/third_party/zlib && mkdir build && cd build && cmake -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE .. && make ${BUILD_THREADS} && make install && \
        cd /dependencies/grpc/third_party/protobuf/cmake && mkdir build && cd build && cmake -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE -Dprotobuf_BUILD_TESTS=OFF .. && make ${BUILD_THREADS} && make install && \
        cd ../../../abseil-cpp && mkdir build && cd build && cmake -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE .. && make ${BUILD_THREADS} && make install && \
        cd ../../re2/ && mkdir build && cd build && cmake -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE .. && make ${BUILD_THREADS} && make install && \
        cd /dependencies/grpc/cmake && mkdir build && cd build && cmake -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF -DgRPC_ABSL_PROVIDER=package \
            -DgRPC_CARES_PROVIDER=package -DgRPC_PROTOBUF_PROVIDER=package \
            -DgRPC_RE2_PROVIDER=package -DgRPC_SSL_PROVIDER=package \
            -DgRPC_ZLIB_PROVIDER=package -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE ../.. && make ${BUILD_THREADS} && make install && \
    cd /dependencies/opencv && mkdir build && cd build && cmake -D BUILD_PERF_TESTS=OFF -D BUILD_TESTS=OFF .. && make ${BUILD_THREADS} && make install && \
    cd /dependencies/ && tar -xvzf zlib-1.2.13.tar.gz && cd zlib-1.2.13 && ./configure && make ${BUILD_THREADS} && make install && \
    cd /dependencies/ && tar -xvf 2.14.0.tar.gz && cd TileDB-2.14.0 && mkdir build && cd build && \
        ../bootstrap --prefix=/usr/local/ && make ${BUILD_THREADS} && make install-tiledb && \
    cd /usr/src/gtest && cmake . && make ${BUILD_THREADS} && mv lib/libgtest* /usr/lib/ && \
    cd /dependencies/valijson && cp -r include/* /usr/local/include/ && \
    cd /dependencies/aws-sdk-cpp && git checkout 276ee83080fcc521d41d456dbbe61d49392ddf77 && cd .. && mkdir aws_sdk_build && cd aws_sdk_build && \
    cmake ../aws-sdk-cpp -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/usr/local/ -DCMAKE_INSTALL_PREFIX=/usr/local/ -DBUILD_ONLY="s3" \
        -DCUSTOM_MEMORY_MANAGEMENT=OFF && make && make install && \
    rm -rf /dependencies


# VDMS
WORKDIR /vdms
RUN git clone https://github.com/IntelLabs/vdms.git /vdms && cd /vdms && \
    git checkout develop && git submodule update --init --recursive && \
    mkdir build && cd build && cmake .. && make ${BUILD_THREADS} && \
    cp /vdms/config-vdms.json /vdms/build/ && \
    echo '#!/bin/bash' > /start.sh && echo 'cd /vdms/build' >> /start.sh && \
    echo './vdms' >> /start.sh && chmod 755 /start.sh

CMD ["/start.sh"]
