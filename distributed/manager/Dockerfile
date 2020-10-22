FROM ubuntu:20.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y cmake python3 python3-pip openjdk-11-jdk libprotobuf-java git && git clone https://github.com/omp87/vdms.git && pip3 install vdms

RUN cd /vdms/distributed/manager && cp ../../utils/src/protobuf/queryMessage.proto ./ && mkdir build && cd build && cmake .. && make -j16




#ENTRYPOINT ["java", "TestServer"]
ENTRYPOINT ["tail", "-f"]
