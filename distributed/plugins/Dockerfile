# Pull base image.
FROM ubuntu:20.04 as plugin_base
#needed so jdk installs without prompting user for response
ENV DEBIAN_FRONTEND=noninteractive

#Install Packages
RUN apt-get update && apt-get -y install maven g++ cmake python3 python3-pip openjdk-11-jdk-headless libprotobuf-java git protobuf-compiler curl unzip && cd / && git clone https://github.com/omp87/vdms.git && git clone https://github.com/google/protobuf.git && cd /protobuf/cmake/ && mkdir build && cd build &&  cmake -Dprotobuf_BUILD_TESTS=OFF .. && make -j8 && cp protoc ../../src/protoc && cd /protobuf/java/core && mvn package && cp $(ls target/protobuf-java*.jar) /usr/share/java/protobuf.jar 


ENTRYPOINT ["tail", "-f"]