# Pull base image.
FROM ubuntu:18.04

#Install Packages
RUN apt-get update && apt-get -y install software-properties-common && add-apt-repository "deb http://security.ubuntu.com/ubuntu xenial-security main" && apt-get -y install g++ git libssl-dev libc-ares-dev apt-transport-https ca-certificates curl gnupg-agent software-properties-common cmake python3-pip build-essential autoconf automake libtool g++ unzip bzip2 bsdtar cmake git pkg-config python python-dev python-pip python-virtualenv wget libbz2-dev libssl-dev liblz4-dev mpich libjsoncpp-dev flex javacc bison openjdk-8-jdk libleveldb-dev libsnappy-dev libhdf5-serial-dev libatlas-base-dev libboost-all-dev libgflags-dev libgoogle-glog-dev liblmdb-dev libjpeg8-dev libtiff5-dev libjasper-dev libgtk-3-dev libopenmpi-dev libgtest-dev ed libgtk2.0-dev pkg-config libavcodec-dev libavformat-dev libswscale-dev libtbb2 libtbb-dev libjpeg-dev libpng-dev libtiff-dev libdc1394-22-dev libopenblas-dev && pip3 install numpy && pip install numpy

#Pull Dependencies
RUN git clone https://github.com/grpc/grpc.git && git clone https://github.com/glennrp/libpng.git && git clone https://github.com/swig/swig.git && git clone https://github.com/opencv/opencv.git && git clone https://github.com/tristanpenman/valijson.git && git clone https://github.com/Kitware/CMake.git && git clone https://github.com/facebookresearch/faiss.git && curl http://zlib.net/zlib-1.2.11.tar.gz -o zlib-1.2.11.tar.gz 

RUN cd /CMake && ./bootstrap && make -j16 && make install && cd / && cd swig && \ 
./autogen.sh && ./configure && make -j16 && make install && cd /faiss && mkdir build && cd build && \
cmake -DFAISS_ENABLE_GPU=OFF .. && make -j16 && make install

RUN cd /grpc && git submodule update --init --recursive && \
cd third_party/protobuf/cmake && mkdir build && cd build && cmake -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE .. && \
make -j8 && make install && cd ../../../abseil-cpp && mkdir build && cd build && \
cmake -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE .. && make -j8 && make install && cd ../../re2/ && mkdir build && \
cd build && cmake -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE .. && make -j8 && make install && cd ../../zlib/ && \
mkdir build && cd build && cmake -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE .. && make -j8 && make install && \
cd /grpc/cmake && mkdir build && cd build && cmake -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF -DgRPC_ABSL_PROVIDER=package \
-DgRPC_CARES_PROVIDER=package -DgRPC_PROTOBUF_PROVIDER=package -DgRPC_RE2_PROVIDER=package -DgRPC_SSL_PROVIDER=package \
-DgRPC_ZLIB_PROVIDER=package -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE ../.. && make -j8 && make install && cd / 

RUN cd / && gunzip zlib-1.2.11.tar.gz && tar -xvf zlib-1.2.11.tar && cd zlib-1.2.11 && ./configure && make -j4 && \
make install && cd / && rm -rf zlib-1.2.11.tar zlib-1.2.11 && cd libpng && git checkout libpng12 && ./configure &&\
make -j4 && make install


# Google Test
RUN cd /usr/src/gtest && cmake . && make -j16 && mv libgtest* /usr/lib/ && cd / && cd /opencv && mkdir build && \
 cd build && cmake -D BUILD_PERF_TESTS=OFF -D BUILD_TESTS=OFF .. && make -j16 && make install

# TileDB
RUN cd / && wget https://github.com/TileDB-Inc/TileDB/archive/1.3.1.tar.gz && \
    tar xf 1.3.1.tar.gz && rm 1.3.1.tar.gz && \
    cd TileDB-1.3.1 && mkdir build && cd build && \
    ../bootstrap --prefix=/usr/local/ && make -j4 && make install-tiledb && \
    rm -rf /TileDB-1.3.1

# Valijson
RUN cd /valijson && cp -r include/* /usr/local/include/ 


RUN cd / && mkdir -p /root/.ssh
ADD id_rsa /root/.ssh/
RUN chmod 700 /root/.ssh/id_rsa*
RUN echo "Host github.com\n\tStrictHostKeyChecking no\n" >> /root/.ssh/config

RUN git clone git@github.com:omp87/vdms.git && cd vdms && git submodule update --init --recursive
RUN cd /vdms && mkdir build && cd build && cmake .. && make -j16 && cd / rm -rf valijson && rm -rf faiss && \
rm -rf grpc && rm -rf libpng  && rm -rf opencv && rm -rf swig && rm -rf CMake

#ENTRYPOINT ["/bin/bash"]


RUN echo '#!/bin/bash' > /start.sh && \
    echo 'cd /vdms/build'   >> /start.sh && \
    echo 'cp ../config-vdms.json ./'   >> /start.sh && \
    echo './vdms'       >> /start.sh && \
    chmod 755 /start.sh

EXPOSE 55555
CMD ["/start.sh"]
