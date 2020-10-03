FROM ubuntu:20.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y python3 python3-pip && pip3 install vdms
RUN apt-get install -y emacs

RUN apt-get install -y default-jdk 
COPY ./QueueServiceThread.java .
COPY ./ServerServiceThread.java .
COPY ./TestPlugin.java .
COPY ./VdmsTransaction.java .

RUN javac QueueServiceThread.java ServerServiceThread.java TestPlugin.java VdmsTransaction.java

#ENTRYPOINT ["java", "RepeaterPlugin"]
ENTRYPOINT ["tail", "-f"]

