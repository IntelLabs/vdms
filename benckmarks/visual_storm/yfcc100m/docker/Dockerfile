# Pull base image.
FROM ubuntu:16.04

RUN apt-get -qq update
RUN apt-get -qq install -y python python-dev python-pip python-virtualenv
RUN apt-get -qq install -y bzip2
RUN apt-get install --no-install-recommends -y libjsoncpp-dev build-essential cmake git pkg-config libprotobuf-dev libleveldb-dev libsnappy-dev libhdf5-serial-dev protobuf-compiler libatlas-base-dev libboost-all-dev libgflags-dev libgoogle-glog-dev liblmdb-dev libgflags-dev libgoogle-glog-dev liblmdb-dev
RUN apt-get install -y scons libjsoncpp-dev flex javacc libbison-dev openjdk-8-jdk
RUN apt-get install -y autoconf automake libtool curl make g++ unzip
RUN apt-get install -y cmake wget git zlib1g-dev libbz2-dev libssl-dev liblz4-dev mpich
RUN apt-get install -y libjpeg8-dev libtiff5-dev libjasper-dev libpng12-dev libgtk-3-dev
RUN apt-get install -y zlib1g-dev
RUN apt-get install -y libbz2-dev
RUN apt-get install -y libssl-dev
RUN apt-get install -y liblz4-dev
RUN apt-get install -y autoconf automake libtool curl make g++ unzip
RUN apt-get install -y libopenmpi-dev
RUN apt-get install -y libgtest-dev
RUN apt-get install -y ed
RUN apt-get install -y libgtk2.0-dev pkg-config libavcodec-dev libavformat-dev libswscale-dev
RUN apt-get install -y libtbb2 libtbb-dev libjpeg-dev libpng-dev libtiff-dev libjasper-dev libdc1394-22-dev
RUN pip install scons numpy
RUN apt-get remove -y libprotobuf-dev protobuf-compiler
RUN cd / && git clone https://github.com/google/protobuf.git
RUN cd protobuf && git checkout v3.3.0 && ./autogen.sh && ./configure --prefix=/usr/local && make -j $(cat /proc/cpuinfo | wc -l) && make install && ldconfig
RUN rm -rf /protobuf

# Blosc
RUN cd / && git clone https://github.com/Blosc/c-blosc.git
RUN cd c-blosc && mkdir build && cd build && cmake -DCMAKE_INSTALL_PREFIX='/usr' .. && cmake --build . && ctest && cmake --build . --target install
RUN rm -rf /c-blosc

# Zstd
RUN cd / && wget https://github.com/facebook/zstd/archive/v1.1.0.tar.gz && tar xf v1.1.0.tar.gz
RUN cd zstd-1.1.0 && make install PREFIX='/usr'
RUN cd /usr/src/gtest && cmake . && make && mv libgtest* /usr/lib/

# TileDB
RUN cd / && wget https://github.com/TileDB-Inc/TileDB/archive/0.6.1.tar.gz && tar xf 0.6.1.tar.gz
RUN cd TileDB-0.6.1 && mkdir build && cd build && cmake .. && make -j $(cat /proc/cpuinfo | wc -l) && make install
RUN rm -rf /TileDB-0.6.1

# Valijson
RUN cd / && git clone https://github.com/tristanpenman/valijson.git
RUN cd valijson && cp -r include/* /usr/local/include/

# OpenCV
RUN mkdir /opencv && cd /opencv && wget -O opencv.zip https://github.com/opencv/opencv/archive/3.3.0.zip
RUN cd /opencv && unzip opencv.zip && cd opencv-3.3.0 && mkdir build && cd build && cmake -D CMAKE_BUILD_TYPE=Release -D CMAKE_INSTALL_PREFIX=/usr/local .. && make -j $(cat /proc/cpuinfo | wc -l) && make install && rm -rf /opencv

# VCL install
RUN cd / && wget https://github.com/intellabs/vcl/archive/v0.1.0.tar.gz && tar xf v0.1.0.tar.gz
RUN mv vcl-0.1.0 vcl && cd vcl && scons -j16

# PMGD install
RUN cd / && wget https://github.com/intellabs/pmgd/archive/v1.0.0.tar.gz && tar xf v1.0.0.tar.gz
RUN mv pmgd-1.0.0 pmgd && cd pmgd && make
RUN rm -rf /*.tar.gz

# VDMS install
RUN cd / && git clone https://github.com/intellabs/vdms.git && cd vdms && git checkout examples_yfcc100m && scons -j12 INTEL_PATH=/
RUN cd /vdms/examples/yfcc100m && scons -j16 INTEL_PATH=/

RUN rm -rf /*.tar.gz
RUN rm -rf /var/lib/apt/lists/* /root/.cache

RUN pip install --upgrade pip
RUN pip install notebook
RUN pip install protobuf

RUN echo '#!/bin/bash' > /start.sh
RUN echo 'export LD_LIBRARY_PATH=/pmgd/lib:$(find /usr/local/lib/ / -type f -name "*.so" | xargs dirname | sort | uniq | tr "\n" ":")' >> /start.sh
RUN echo 'cd /vdms/examples/yfcc100m' >> /start.sh
RUN echo 'rm -rf db' >> /start.sh
RUN echo 'mkdir db' >> /start.sh
RUN echo 'mkdir db/images' >> /start.sh
RUN echo 'mkdir db/images/pngs' >> /start.sh
RUN echo 'mkdir db/images/jpgs' >> /start.sh
RUN echo 'mkdir db/images/tiledb' >> /start.sh
RUN echo './yfcc --media yfcc100m_dataset_short_10k yfcc100m_pmgd' >> /start.sh
RUN echo '../../vdms &' >> /start.sh
RUN echo 'cd /vdms' >> /start.sh
RUN echo 'export PYTHONPATH=$PYTHONPATH:/vdms/client/python/' >> /start.sh
RUN echo 'jupyter notebook --port=8888 --no-browser --allow-root --ip=0.0.0.0' >> /start.sh
RUN chmod 755 /start.sh

# Add Tini. Tini operates as a process subreaper for jupyter. This prevents
# kernel crashes.
ENV TINI_VERSION v0.6.0
ADD https://github.com/krallin/tini/releases/download/${TINI_VERSION}/tini /usr/bin/tini
RUN chmod +x /usr/bin/tini
ENTRYPOINT ["/usr/bin/tini", "--"]

EXPOSE 8888
CMD ["/start.sh"]
