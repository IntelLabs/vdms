FROM ubuntu:20.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y python3 python3-pip libprotobuf-java && pip3 install vdms
RUN apt-get install -y emacs

RUN apt-get install -y default-jdk 
COPY ./TestServer.java .
COPY ./VdmsTransaction.java .
COPY ./ClientServiceThread.java .
COPY ./QueueServiceThread.java .

RUN javac ClientServiceThread.java  QueueServiceThread.java  TestServer.java  VdmsTransaction.java

#ENTRYPOINT ["java", "TestServer"]
ENTRYPOINT ["tail", "-f"]
