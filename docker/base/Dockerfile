# Pull base image.
FROM ubuntu:18.04

RUN apt-get update && apt-get -y install software-properties-common g++ git cmake libssl-dev libc-ares-dev && cd / cd / && git clone https://github.com/grpc/grpc.git && cd grpc && git submodule update --init --recursive && cd third_party/protobuf/cmake && mkdir build && cd build && cmake -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE .. && make -j8 && make install && cd ../../../abseil-cpp && mkdir build && cd build && cmake -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE .. && make -j8 && make install && cd ../../re2/ && mkdir build && cd build && cmake -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE .. && make -j8 && make install && cd ../../zlib/ && mkdir build && cd build && cmake -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE .. && make -j8 && make install && cd /grpc/cmake && mkdir build && cd build && cmake -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF -DgRPC_ABSL_PROVIDER=package -DgRPC_CARES_PROVIDER=package -DgRPC_PROTOBUF_PROVIDER=package -DgRPC_RE2_PROVIDER=package -DgRPC_SSL_PROVIDER=package -DgRPC_ZLIB_PROVIDER=package -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE ../.. && make -j8 && make install && cd / && mkdir -p /root/.ssh

RUN add-apt-repository "deb http://security.ubuntu.com/ubuntu xenial-security main" && apt-get install -y apt-transport-https ca-certificates curl gnupg-agent software-properties-common cmake python3-pip git && curl http://zlib.net/zlib-1.2.11.tar.gz -o zlib-1.2.11.tar.gz && gunzip zlib-1.2.11.tar.gz && tar -xvf zlib-1.2.11.tar && cd zlib-1.2.11 && ./configure && make -j4 && make install && cd / && rm -rf zlib-1.2.11.tar zlib-1.2.11 && git clone https://github.com/glennrp/libpng.git && cd libpng && git checkout libpng12 && ./configure && make -j4 && make install && cd / && apt-get -y install build-essential scons autoconf automake libtool g++ unzip bzip2 bsdtar cmake git pkg-config python python-dev python-pip python-virtualenv wget zlib1g-dev libbz2-dev libssl-dev liblz4-dev mpich libjsoncpp-dev flex javacc bison openjdk-8-jdk libleveldb-dev libsnappy-dev libhdf5-serial-dev libatlas-base-dev libboost-all-dev libgflags-dev libgoogle-glog-dev liblmdb-dev libjpeg8-dev libtiff5-dev libjasper-dev libgtk-3-dev libopenmpi-dev libgtest-dev ed libgtk2.0-dev pkg-config libavcodec-dev libavformat-dev libswscale-dev libtbb2 libtbb-dev libjpeg-dev libpng-dev libtiff-dev libdc1394-22-dev libopenblas-dev && apt-get remove -y libprotobuf-dev protobuf-compiler #

# Google Test
RUN cd /usr/src/gtest && cmake . && make && mv libgtest* /usr/lib/


RUN cd / && git clone git@github.com:opencv/opencv.git && cd opencv && mkdir build && cd build && cmake -D BUILD_PERF_TESTS=OFF -D BUILD_TESTS=OFF .. && make -j16 && make install

# TileDB
RUN wget https://github.com/TileDB-Inc/TileDB/archive/1.3.1.tar.gz && \
    tar xf 1.3.1.tar.gz && rm 1.3.1.tar.gz && \
    cd TileDB-1.3.1 && mkdir build && cd build && \
    ../bootstrap --prefix=/usr/local/ && make -j4 && make install-tiledb && \
    rm -rf /TileDB-1.3.1

# Faiss
RUN wget https://github.com/facebookresearch/faiss/archive/v1.4.0.tar.gz && \
    tar xf v1.4.0.tar.gz && rm v1.4.0.tar.gz && \
    cd faiss-1.4.0 && rm CMakeLists.txt && \
    wget https://gist.githubusercontent.com/luisremis/758c71c6a86cb3e4fe2311c415e07547/raw/3c51b5ed7d3f16a4d684d9ea524602c9f1f8593a/CMakeLists.txt && \
    mkdir build && cd build && cmake ../ && make -j4 && \
    mkdir /usr/local/include/faiss/ && \
    cp -r /faiss-1.4.0/* /usr/local/include/faiss/ && \
    mv /faiss-1.4.0/build/lib/libfaiss.so /usr/local/lib/ && \
    rm -rf /faiss-1.4*

# Valijson
RUN git clone https://github.com/tristanpenman/valijson.git && \
    cd valijson && cp -r include/* /usr/local/include/ && \
    rm -r /valijson



ADD id_rsa /root/.ssh/
RUN chmod 700 /root/.ssh/id_rsa*
RUN echo "Host github.com\n\tStrictHostKeyChecking no\n" >> /root/.ssh/config
RUN git clone git@github.com:omp87/vdms.git && cd vdms && git submodule update --init --recursive
RUN cd /vdms && mkdir build && cd build && cmake .. && make -j16

#ENTRYPOINT ["/bin/bash"]


RUN echo '#!/bin/bash' > /start.sh && \
    echo 'cd /vdms'   >> /start.sh && \
    echo 'vdms'       >> /start.sh && \
    chmod 755 /start.sh

EXPOSE 55555
CMD ["/start.sh"]
