FROM ubuntu:20.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y python3 python3-pip libprotobuf-java && pip3 install vdms
RUN apt-get install -y emacs

RUN apt-get install -y default-jdk 
COPY ./QueueServiceThread.java .
COPY ./PublisherServiceThread.java .
COPY ./SubscriberServiceThread.java .
COPY ./TestPlugin.java .
COPY ./VdmsTransaction.java .

RUN javac QueueServiceThread.java SubscriberServiceThread.java PublisherServiceThread.java TestPlugin.java VdmsTransaction.java

#ENTRYPOINT ["java", "RepeaterPlugin"]
ENTRYPOINT ["tail", "-f"]

