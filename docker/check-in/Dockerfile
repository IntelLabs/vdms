#Copyright (C) 2023 Intel Corporation
#SPDX-License-Identifier: MIT

ARG BASE_VERSION=11.8-slim
ARG BUILD_THREADS="-j16"
############################################################
# BASE IMAGE W/ ENV VARS
FROM debian:${BASE_VERSION} as base
# Dockerfile limitations force a repetition of global args
ARG BUILD_THREADS

ENV DEBIAN_FRONTEND=noninteractive
ENV DEBCONF_NOWARNINGS="yes"
ENV PROTOBUF_VERSION="24.2"
ENV NUMPY_MIN_VERSION="1.26.0"

############################################################
# BUILD DEPENDENCIES
FROM base as build

# Install Packages
# hadolint ignore=DL3008
RUN apt-get update -y && apt-get upgrade -y && \
    apt-get install -o 'Acquire::Retries=3' -y --no-install-suggests --no-install-recommends --fix-broken --fix-missing \
        apt-transport-https autoconf automake bison build-essential bzip2 ca-certificates \
        curl ed flex g++-9 gcc-9 git gnupg-agent javacc libarchive-tools libatlas-base-dev \
        libavcodec-dev libavformat-dev libboost-all-dev libbz2-dev libc-ares-dev libcurl4-openssl-dev \
        libdc1394-22-dev libgflags-dev libgoogle-glog-dev libgtk-3-dev libgtk2.0-dev \
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
WORKDIR /dependencies
ENV CMAKE_VERSION="v3.27.2"  \
    VALIJSON_VERSION="v0.6" \
    FAISS_VERSION="v1.7.4" \
    OPENCV_VERSION="4.5.5" \
    TILEDB_VERSION="2.14.1" \
    AWS_SDK_VERSION="1.11.0" \
    AUTOCONF_VERSION="2.71" \
    PEG_VERSION="0.1.19"

# hadolint ignore=DL3003
RUN python3 -m pip install --no-cache-dir "numpy>=${NUMPY_MIN_VERSION}" && \
    git clone --branch ${VALIJSON_VERSION} https://github.com/tristanpenman/valijson.git /dependencies/valijson && \
        cd /dependencies/valijson && cp -r include/* /usr/local/include/ && \
        mkdir -p /opt/dist/usr/local/include/ && cp -r include/* /opt/dist/usr/local/include/

# hadolint ignore=DL3003,SC2086
RUN git clone --branch ${CMAKE_VERSION} https://github.com/Kitware/CMake.git /dependencies/CMake && \
        cd /dependencies/CMake && ./bootstrap && make ${BUILD_THREADS} && \
            make install DESTDIR=/opt/dist && make install

# PROTOBUF & ITS DEPENDENCIES
# hadolint ignore=DL3003,SC2086
RUN git clone -b "v${PROTOBUF_VERSION}" --recurse-submodules https://github.com/protocolbuffers/protobuf.git /dependencies/protobuf && \
        cd /dependencies/protobuf/third_party/googletest && mkdir build && cd build/ && \
            cmake -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local -DBUILD_GMOCK=ON -DCMAKE_CXX_STANDARD=17 .. && \
            make ${BUILD_THREADS} && make install DESTDIR=/opt/dist && make install && ldconfig && \
        cd /dependencies/protobuf/third_party/abseil-cpp &&  mkdir build && cd build && \
            cmake -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_PREFIX_PATH=/usr/local/ -DCMAKE_INSTALL_PREFIX=/usr/local/ -DABSL_BUILD_TESTING=ON \
                -DABSL_ENABLE_INSTALL=ON -DABSL_USE_EXTERNAL_GOOGLETEST=ON -DABSL_FIND_GOOGLETEST=ON -DCMAKE_CXX_STANDARD=17 .. && \
            make ${BUILD_THREADS} && make install DESTDIR=/opt/dist && make install && \
        cd /dependencies/protobuf && \
            cmake -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_CXX_STANDARD=17 \
                -Dprotobuf_ABSL_PROVIDER=package -DCMAKE_PREFIX_PATH=/usr/local . && \
            make ${BUILD_THREADS} && make install DESTDIR=/opt/dist && make install

# DESCRIPTOR LIBRARIES
# hadolint ignore=DL3003,SC2086
RUN git clone --branch ${FAISS_VERSION} https://github.com/facebookresearch/faiss.git /dependencies/faiss && \
        cd /dependencies/faiss && mkdir build && cd build && \
            cmake -DFAISS_ENABLE_GPU=OFF -DPython_EXECUTABLE=/usr/bin/python3 .. && \
            make ${BUILD_THREADS} && make install DESTDIR=/opt/dist && make install && \
    git clone https://github.com/tonyzhang617/FLINNG.git /dependencies/FLINNG && \
        cd /dependencies/FLINNG && mkdir build && cd build && cmake .. && \
            make ${BUILD_THREADS} && make install DESTDIR=/opt/dist && make install

# TILEDB & AWS S3 SDK
# hadolint ignore=DL3003,SC2086
RUN curl -L -o /dependencies/${TILEDB_VERSION}.tar.gz \
        https://github.com/TileDB-Inc/TileDB/archive/refs/tags/${TILEDB_VERSION}.tar.gz && \
        cd /dependencies/ && tar -xvf ${TILEDB_VERSION}.tar.gz && cd TileDB-${TILEDB_VERSION} && \
            mkdir build && cd build && ../bootstrap --prefix=/usr/local/ && make ${BUILD_THREADS} && \
            make install-tiledb DESTDIR=/opt/dist && make install-tiledb && \
    git clone -b ${AWS_SDK_VERSION} --recurse-submodules https://github.com/aws/aws-sdk-cpp /dependencies/aws-sdk-cpp && \
        mkdir -p /dependencies/aws-sdk-cpp/build && cd /dependencies/aws-sdk-cpp/build && \
        cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/usr/local/ -DCMAKE_INSTALL_PREFIX=/usr/local/ \
            -DBUILD_ONLY="s3" -DCUSTOM_MEMORY_MANAGEMENT=OFF -DENABLE_TESTING=OFF && \
        make ${BUILD_THREADS} && make install DESTDIR=/opt/dist && make install

# OPENCV
# hadolint ignore=DL3003,SC2086
RUN git clone --branch ${OPENCV_VERSION} https://github.com/opencv/opencv.git /dependencies/opencv && \
        cd /dependencies/opencv && mkdir build && cd build && cmake -D BUILD_PERF_TESTS=OFF -D BUILD_TESTS=OFF .. && \
            make ${BUILD_THREADS} && make install DESTDIR=/opt/dist && make install

# LIB-OMNI FOR NEO4J QUERY HANDLER
# hadolint ignore=DL3003,SC2086
RUN curl -O  https://ftp.gnu.org/gnu/autoconf/autoconf-${AUTOCONF_VERSION}.tar.xz && \
        tar -xf autoconf-${AUTOCONF_VERSION}.tar.xz && cd autoconf-${AUTOCONF_VERSION} && \
        ./configure && make ${BUILD_THREADS} && make install DESTDIR=/opt/dist && make install && \
    curl -L -o /dependencies/peg-${PEG_VERSION}.tar.gz \
        https://github.com/gpakosz/peg/releases/download/${PEG_VERSION}/peg-${PEG_VERSION}.tar.gz && \
        cd /dependencies/ && tar -xf peg-${PEG_VERSION}.tar.gz && cd peg-${PEG_VERSION} && \
        make ${BUILD_THREADS} && make install DESTDIR=/opt/dist && make install && \
    git clone https://github.com/cleishm/libcypher-parser.git /dependencies/libcypher && \
        cd /dependencies/libcypher && ./autogen.sh && ./configure && \
        make install DESTDIR=/opt/dist && make install && \
    git clone https://github.com/majensen/libneo4j-omni.git /dependencies/libomni && \
        cd /dependencies/libomni && ./autogen.sh && ./configure --disable-tools && \
        make install DESTDIR=/opt/dist -no-werror && make install -no-werror

# CLEANUP
RUN rm -rf /dependencies /usr/local/share/doc /usr/local/share/man && \
    mkdir -p /opt/dist/usr/include/x86_64-linux-gnu && \
        cp -rp /usr/include/x86_64-linux-gnu /opt/dist/usr/include/x86_64-linux-gnu


############################################################
# FINAL IMAGE
FROM base
ARG BUILD_COVERAGE="on"
ARG BUILD_COVERITY="off"

# hadolint ignore=DL3008
RUN apt-get update -y && apt-get upgrade -y && \
    apt-get install -o 'Acquire::Retries=3' -y --no-install-suggests --no-install-recommends --fix-broken --fix-missing \
        build-essential bzip2 curl g++-9 gcc-9 git javacc libarchive-tools libavcodec-dev libavformat-dev libcurl4-openssl-dev \
        libdc1394-22-dev libgoogle-glog-dev libgtk-3-dev libgtk2.0-dev libhdf5-dev libjpeg-dev libjpeg62-turbo-dev libjsoncpp-dev libopenblas-dev \
        libpng-dev librdkafka-dev libssl-dev libswscale-dev libtbb-dev libtbb2 libtiff-dev libtiff5-dev libzmq3-dev  openjdk-11-jdk-headless procps python3-dev python3-pip && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 1 && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-9 1 && \
    apt-get clean && rm -rf /var/lib/apt/lists/* && \
    ln -s /usr/bin/python3 /usr/bin/python && \
    python3 -m pip install --no-cache-dir "numpy>=${NUMPY_MIN_VERSION}" "coverage>=7.3.1" "protobuf==4.${PROTOBUF_VERSION}"

COPY --from=build /opt/dist /
RUN echo "/usr/local/lib" >> /etc/ld.so.conf.d/all-libs.conf && ldconfig

# COVERITY & MINIO for S3 Testing
ENV COVERITY_VERSION="2023.3.4" \
    PATH=/opt/coverity/analysis/bin:$PATH
WORKDIR /coverity
RUN if [ "${BUILD_COVERITY}" = "on" ]; then \
        mkdir -p /coverity /opt/coverity ; \
        curl -L -o /coverity/cov-analysis-linux64-${COVERITY_VERSION}.sh  https://ubit-artifactory-or.intel.com/artifactory/coverity-or-local/Enterprise/cov-analysis-linux64-${COVERITY_VERSION}.sh ; \
        curl -L -o /coverity/license.dat https://ubit-artifactory-or.intel.com/artifactory/coverity-or-local/Enterprise/license.dat ; \
        chmod +x /coverity/cov-analysis-linux64-${COVERITY_VERSION}.sh ; \
        ./cov-analysis-linux64-${COVERITY_VERSION}.sh -q \
            --installation.dir=/opt/coverity/analysis \
            --license.agreement=agree \
            --license.region=0 \
            --license.type.choice=0 \
            --license.cov.path=/coverity/license.dat \
            --component.sdk=false \
            --component.skip.documentation=true; \
        rm /coverity/cov-analysis-linux64-${COVERITY_VERSION}.sh ; \
    fi

# COVERAGE TESTING
COPY ./docker/check-in/run_coverage_cpp.sh /
COPY ./docker/check-in/run_coverage_py.sh /
WORKDIR /vdms
# hadolint ignore=DL3008
RUN if [ "${BUILD_COVERAGE}" = "on" ]; then \
        apt-get update -y ; \
        apt-get install -y --no-install-suggests --no-install-recommends gdb ; \
        apt-get clean ; \
        rm -rf /var/lib/apt/lists/* ; \
        pip3 install --no-cache-dir "gcovr>=6.0" ; \
        curl -L -o /vdms/minio https://dl.min.io/server/minio/release/linux-amd64/minio ; \
        chmod +x /vdms/minio ; \
        mkdir -p /vdms/minio_files/minio-bucket ; \
        mkdir -p /vdms/tests/coverage_report ; \
        chmod +x /run_coverage_*.sh ; \
        # Install the MinIO Client mc command line tool used by scripts for creating buckets
        curl -o /usr/local/bin/mc https://dl.min.io/client/mc/release/linux-amd64/mc ; \
        chmod +x /usr/local/bin/mc ; \
    else \
        rm -rf /run_coverage_*.sh ; \
    fi

# VDMS
COPY ./.git /vdms/.git
COPY ./client /vdms/client
COPY ./distributed /vdms/distributed
COPY ./include /vdms/include
COPY ./src /vdms/src
COPY ./tests /vdms/tests
COPY ./utils /vdms/utils
COPY ./CMakeLists.txt /vdms/
COPY ./config-vdms.json /vdms/
# hadolint ignore=DL3003,SC2086
RUN git submodule update --init --recursive && \
        mkdir -p /vdms/build && cd /vdms/build && \
        cmake -DCODE_COVERAGE="${BUILD_COVERAGE}" .. && make ${BUILD_THREADS} && \
    cp /vdms/config-vdms.json /vdms/build/ && \
    echo '#!/bin/bash' > /start.sh && echo 'cd /vdms/build' >> /start.sh && \
    echo './vdms' >> /start.sh && chmod 755 /start.sh

ENV PYTHONPATH=/vdms/client/python:${PYTHONPATH}
HEALTHCHECK CMD echo "This is a healthcheck test." || exit 1
CMD ["/start.sh"]
