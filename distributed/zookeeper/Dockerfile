FROM ubuntu:20.04
RUN apt-get update && apt-get install -y curl openjdk-11-jdk && curl https://apache.claz.org/zookeeper/zookeeper-3.6.2/apache-zookeeper-3.6.2-bin.tar.gz -o /opt/apache-zookeeper-3.6.2-bin.tar.gz && cd /opt && gunzip apache-zookeeper-3.6.2-bin.tar.gz && tar -xf apache-zookeeper-3.6.2-bin.tar && cd apache-zookeeper-3.6.2-bin && mkdir data

COPY zoo.cfg /opt/apache-zookeeper-3.6.2-bin/conf
COPY start.sh /start.sh
RUN chmod 755 /start.sh
ENV JAVA_HOME="/usr/lib/jvm/java-11-openjdk-amd64"

ENTRYPOINT ["bash", "/start.sh"]