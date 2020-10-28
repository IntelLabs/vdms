FROM ubuntu:20.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y cmake python3 python3-pip openjdk-11-jdk libprotobuf-java git protobuf-compiler curl unzip && git clone https://github.com/omp87/vdms.git && pip3 install vdms && curl http://www.java2s.com/Code/JarDownload/json-simple/json-simple-1.1.1.jar.zip -o json-simple-1.1.1.jar.zip && unzip json-simple-1.1.1.jar.zip && mv json-simple-1.1.1.jar /usr/share/java && rm json-simple-1.1.1.jar.zip

RUN cd /vdms && git checkout origin/distributed_vdms && cd distributed/plugins &&  mkdir build && cd build && cp ../../../utils/src/protobuf/queryMessage.proto ./ && cmake .. && make -j16

#ENTRYPOINT ["java", "-cp", "TestPlugin.jar:/usr/share/java/protobuf.jar", "TestPlugin"]
ENTRYPOINT ["tail", "-f"]
