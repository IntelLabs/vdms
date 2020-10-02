FROM ubuntu:20.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y python3 python3-pip && pip3 install vdms
RUN apt-get install -y emacs

RUN apt-get install -y default-jdk 
COPY ./TestServer.java .
COPY ./WorkerConnection.java .
COPY ./WorkerConnectionList.java .
COPY ./VdmsTransaction.java .
RUN javac TestServer.java WorkerConnection.java WorkerConnectionList.java VdmsTransaction.java


#ENTRYPOINT ["java", "TestServer"]
ENTRYPOINT ["tail" "-f"]