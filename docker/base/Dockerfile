#Copyright (C) 2023 Intel Corporation
#SPDX-License-Identifier: MIT

ARG BASE_VERSION=11.7-slim
ARG BUILD_THREADS="-j16"

FROM debian:${BASE_VERSION}

# Dockerfile limitations force a repetition of global args
ARG BUILD_THREADS

# Install Packages
RUN apt-get update && apt-get install -y --no-install-suggests --no-install-recommends \
    apt-transport-https autoconf automake bison build-essential bzip2 ca-certificates \
    curl ed flex g++-9 gcc-9 git gnupg-agent javacc libarchive-tools libatlas-base-dev \
    libavcodec-dev libavformat-dev libboost-all-dev libbz2-dev libc-ares-dev libcurl4-openssl-dev \
    libdc1394-22-dev libgflags-dev libgoogle-glog-dev libgtest-dev libgtk-3-dev libgtk2.0-dev \
    libhdf5-dev libjpeg-dev libjpeg62-turbo-dev libjsoncpp-dev libleveldb-dev liblmdb-dev \
    liblz4-dev libopenblas-dev libopenmpi-dev libpng-dev librdkafka-dev libsnappy-dev libssl-dev \
    libswscale-dev libtbb-dev libtbb2 libtiff-dev libtiff5-dev libtool libzmq3-dev linux-libc-dev mpich \
    openjdk-11-jdk-headless pkg-config procps python3-dev python3-pip software-properties-common \
    swig unzip uuid-dev && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 1 && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-9 1 && \
    apt-get clean && rm -rf /var/lib/apt/lists/* && \
    ln -s /usr/bin/python3 /usr/bin/python

# Pull and Install Dependencies
ENV CMAKE_VERSION="v3.26.4" \
    PROTOBUF_VERSION="3.20.3" \
    OPENCV_VERSION="4.5.5" \
    FAISS_VERSION="v1.7.3" \
    VALIJSON_VERSION="v0.6" \
    AWS_SDK_VERSION="1.11.0" \
    TILEDB_VERSION="2.14.1"

WORKDIR /dependencies
RUN pip install --no-cache-dir "numpy>=1.25.1" "protobuf==${PROTOBUF_VERSION}" "coverage>=7.2.7" && \
    git clone --branch ${CMAKE_VERSION} https://github.com/Kitware/CMake.git && \
        cd CMake && ./bootstrap && make ${BUILD_THREADS} && make install && \
    cd /usr/src/gtest && cmake . && make ${BUILD_THREADS} && mv lib/libgtest* /usr/lib/ && \
    git clone --branch ${FAISS_VERSION} https://github.com/facebookresearch/faiss.git /dependencies/faiss && \
        cd /dependencies/faiss && mkdir build && cd build && cmake -DFAISS_ENABLE_GPU=OFF .. && \
        make ${BUILD_THREADS} && make install && cd /dependencies/ && \
    git clone https://github.com/tonyzhang617/FLINNG.git && \
        cd /dependencies/FLINNG && mkdir build && cd build && cmake .. && make ${BUILD_THREADS} && make install && cd /dependencies && \
    curl -L -o /dependencies/${PROTOBUF_VERSION}.tar.gz \
        https://github.com/protocolbuffers/protobuf/archive/refs/tags/v${PROTOBUF_VERSION}.tar.gz && \
        cd /dependencies/ && tar -xvf ${PROTOBUF_VERSION}.tar.gz && \
        cd protobuf-${PROTOBUF_VERSION}  && ./autogen.sh && ./configure &&  make -j$(nproc) && \
        make install && ldconfig && cd /dependencies && \
    git clone --branch ${OPENCV_VERSION} https://github.com/opencv/opencv.git && \
        cd opencv && mkdir build && cd build && cmake -D BUILD_PERF_TESTS=OFF -D BUILD_TESTS=OFF .. && \
        make ${BUILD_THREADS} && make install && cd /dependencies/ && \
    git clone --branch ${VALIJSON_VERSION} https://github.com/tristanpenman/valijson.git && \
        cd valijson && cp -r include/* /usr/local/include/ && cd /dependencies && \
    curl -L -o /dependencies/${TILEDB_VERSION}.tar.gz \
        https://github.com/TileDB-Inc/TileDB/archive/refs/tags/${TILEDB_VERSION}.tar.gz && \
        cd /dependencies/ && tar -xvf ${TILEDB_VERSION}.tar.gz && cd TileDB-${TILEDB_VERSION} && \
            mkdir build && cd build && ../bootstrap --prefix=/usr/local/ && make ${BUILD_THREADS} && \
            make install-tiledb && cd /dependencies && \
    git clone -b ${AWS_SDK_VERSION} --recurse-submodules https://github.com/aws/aws-sdk-cpp && \
        mkdir -p aws-sdk-cpp/build && cd aws-sdk-cpp/build && \
        cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/usr/local/ -DCMAKE_INSTALL_PREFIX=/usr/local/ \
            -DBUILD_ONLY="s3" -DCUSTOM_MEMORY_MANAGEMENT=OFF -DENABLE_TESTING=OFF && \
        make ${BUILD_THREADS} && make install && \
    rm -rf /dependencies /usr/local/share/doc /usr/local/share/man


# VDMS
WORKDIR /vdms
RUN git clone -b develop --recurse-submodules https://github.com/IntelLabs/vdms.git /vdms && \
    mkdir -p /vdms/build && cd /vdms/build && cmake .. && make ${BUILD_THREADS} && \
    cp /vdms/config-vdms.json /vdms/build/ && \
    echo '#!/bin/bash' > /start.sh && echo 'cd /vdms/build' >> /start.sh && \
    echo './vdms' >> /start.sh && chmod 755 /start.sh

CMD ["/start.sh"]
