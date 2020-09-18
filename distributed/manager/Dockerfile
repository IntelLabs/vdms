FROM ubuntu:20.04
RUN apt-get update && apt-get install -y python3 python3-pip && pip3 install vdms
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get install -y default-jdk 
COPY ./NewClass.java .
COPY ./WorkerConnection.java .
COPY ./WorkerConnectionList.java .
COPY ./VdmsTransaction.java .
RUN javac NewClass.java WorkerConnection.java WorkerConnectionList.java VdmsTransaction.java


ENTRYPOINT ["tail", "-f"]