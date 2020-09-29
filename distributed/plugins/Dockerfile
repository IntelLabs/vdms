FROM ubuntu:20.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y python3 python3-pip && pip3 install vdms
RUN apt-get install -y emacs

RUN apt-get install -y default-jdk 
COPY ./RepeaterPlugin.java .
COPY ./WorkerConnection.java .
COPY ./SourceConnection.java .
COPY ./WorkerConnectionList.java .
COPY ./VdmsTransaction.java .
RUN javac RepeaterPlugin.java WorkerConnection.java WorkerConnectionList.java VdmsTransaction.java


#ENTRYPOINT ["java", "RepeaterPlugin"]
ENTRYPOINT ["tail", "-f"]