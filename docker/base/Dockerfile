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
ARG MAVEN_OPTS

#Install Packages #2
RUN apt-get update && \
    apt-get -y install software-properties-common && \
    add-apt-repository "deb http://security.ubuntu.com/ubuntu $UBUNTU_NAME-security main" && \
    apt-get -y install \
        g++  \
        git  \
        libssl-dev  \
        libc-ares-dev  \
        apt-transport-https  \
        ca-certificates  \
        curl  \
        gnupg-agent  \
        python3-pip  \
        build-essential  \
        autoconf  \
        automake  \
        libtool  \
        unzip  \
        bzip2  \
        libarchive-tools  \
        pkg-config  \
        python  \
        python-dev  \
        wget  \
        libbz2-dev  \
        liblz4-dev  \
        mpich  \
        libjsoncpp-dev  \
        flex  \
        javacc  \
        bison  \
        openjdk-11-jdk  \
        libleveldb-dev  \
        libsnappy-dev  \
        libhdf5-serial-dev  \
        libatlas-base-dev  \
        libboost-all-dev \
        libgflags-dev \
        libgoogle-glog-dev \
        liblmdb-dev \
        libjpeg8-dev \
        libtiff5-dev \
        libgtk-3-dev \
        libopenmpi-dev \
        libgtest-dev \
        ed \
        libgtk2.0-dev \
        pkg-config \
        libavcodec-dev \
        libavformat-dev \
        libswscale-dev \
        libtbb2 \
        libtbb-dev \
        libjpeg-dev \
        libpng-dev \
        libtiff-dev \
        libdc1394-22-dev \
        libopenblas-dev

RUN pip3 install numpy

# Install the latest version of CMake #3
RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null && \
    apt-add-repository "deb https://apt.kitware.com/ubuntu/ $UBUNTU_NAME main" && \
    apt-get update && apt-get install -y \
        cmake

#Dependencies
RUN git clone https://github.com/swig/swig.git && \
    cd swig && ./autogen.sh && ./configure && make $BUILD_THREADS && make install && \
    cd /


RUN cd /usr/src/gtest && cmake . && make  && mv lib/libgtest* /usr/lib/ && \
    cd / && \
    git clone https://github.com/opencv/opencv.git && \
    cd /opencv && mkdir build && cd build && cmake -DBUILD_PERF_TESTS=OFF -DBUILD_TESTS=OFF .. && \
    make && make install

RUN git clone https://github.com/facebookresearch/faiss.git && \
    cd /faiss && \
    git fetch --all --tags && git checkout tags/v1.7.0 && mkdir build && \
    cd build && cmake -DFAISS_ENABLE_GPU=OFF .. && make && \
    make install && \
    cd /


RUN git clone https://github.com/grpc/grpc.git && \
    cd /grpc && git submodule update --init --recursive

RUN cd /grpc/third_party/protobuf/cmake && mkdir build && cd build && \
    cmake -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE .. && make $BUILD_THREADS && \
    make install

RUN cd /grpc/third_party/abseil-cpp && mkdir build && cd build && \
    cmake -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE .. && make $BUILD_THREADS && make install

RUN cd /grpc/third_party/re2/ && mkdir build && cd build && \
    cmake -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE .. && make $BUILD_THREADS && make install

RUN cd /grpc/third_party/zlib/ && mkdir build && cd build && \
    cmake -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE .. && make $BUILD_THREADS && make install

RUN cd /grpc/cmake && mkdir build && cd build && cmake -DgRPC_INSTALL=ON \
        -DgRPC_BUILD_TESTS=OFF -DgRPC_ABSL_PROVIDER=package \
        -DgRPC_CARES_PROVIDER=package -DgRPC_PROTOBUF_PROVIDER=package \
        -DgRPC_RE2_PROVIDER=package -DgRPC_SSL_PROVIDER=package \
        -DgRPC_ZLIB_PROVIDER=package -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE ../.. && \
    make $BUILD_THREADS && make install && \
    cd /


RUN git clone https://github.com/glennrp/libpng.git && \ 
    cd libpng && \
    git checkout libpng12 && \
    ./configure && make $BUILD_THREADS && make install && \
    cd /

RUN curl http://zlib.net/zlib-1.2.11.tar.gz -o zlib-1.2.11.tar.gz && \
    tar -xvf zlib-1.2.11.tar.gz && cd zlib-1.2.11 && ./configure && make $BUILD_THREADS && \
    make install && \
    cd /

RUN git clone https://github.com/tristanpenman/valijson.git && \
    cd /valijson && cp -r include/* /usr/local/include/ && \
    cd /

RUN curl https://storage.googleapis.com/google-code-archive-downloads/v2/code.google.com/json-simple/json-simple-1.1.1.jar -o /usr/share/java/json-simple-1.1.1.jar

RUN curl https://downloads.apache.org/commons/codec/binaries/commons-codec-1.15-bin.tar.gz -o /commons-codec-1.15-bin.tar.gz && \
gunzip /commons-codec-1.15-bin.tar.gz && \
tar -xvf commons-codec-1.15-bin.tar && \
cp commons-codec-1.15/commons-codec-1.15.jar /usr/share/java/commons-codec-1.15.jar



RUN cd / && wget https://github.com/TileDB-Inc/TileDB/archive/1.3.1.tar.gz && \
    tar xf 1.3.1.tar.gz && rm 1.3.1.tar.gz && \
    cd TileDB-1.3.1 && mkdir build && cd build && \
    ../bootstrap --prefix=/usr/local/ && make $BUILD_THREADS && make install-tiledb && \
    rm -rf /TileDB-1.3.1


RUN apt-get install -y maven

RUN ln -s /grpc/third_party/protobuf/cmake/build/protoc \
    /grpc/third_party/protobuf/src/protoc && \
    cd /grpc/third_party/protobuf/java/core && mvn package && \
    echo "cp $(ls target/protobuf-java*.jar) /usr/share/java/protobuf.jar" > \
    tmp_script && bash tmp_script && \
    cd / 

RUN git clone https://github.com/omp87/vdms.git && cd vdms && \
git checkout develop && git submodule update --init --recursive && \
cd src/pmgd && git checkout develop && cd /vdms && mkdir build && \
cd build && cmake .. && make $BUILD_THREADS

RUN echo '#!/bin/bash' > /start.sh && \
    echo 'cd /vdms/build'   >> /start.sh && \
    echo 'cp ../config-vdms.json ./'   >> /start.sh && \
    echo './vdms'       >> /start.sh && \
    chmod 755 /start.sh

CMD ["/start.sh"]
