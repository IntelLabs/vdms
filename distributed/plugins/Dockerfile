FROM ubuntu:20.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y cmake python3 python3-pip openjdk-11-jdk libprotobuf-java git protobuf-compiler && git clone https://github.com/omp87/vdms.git && pip3 install vdms

RUN cd /vdms && git checkout origin/distributed_vdms && cd distributed/plugins &&  mkdir build && cd build && cp ../../../utils/src/protobuf/queryMessage.proto ./ && cmake .. && make -j16

#ENTRYPOINT ["java", "-cp", "TestPlugin.jar:/usr/share/java/protobuf.jar", "TestPlugin"]
ENTRYPOINT ["tail", "-f"]
