FROM ubuntu:20.04
RUN apt-get update && apt-get install -y curl openjdk-11-jdk && curl https://ftp.wayne.edu/apache/kafka/2.6.0/kafka_2.13-2.6.0.tgz -o /opt/kafka_2.13-2.6.0.tgz && cd /opt && tar -xzf kafka_2.13-2.6.0.tgz && cd kafka_2.13-2.6.0 && mkdir kafka-logs && mkdir zookeeper-data

COPY stub_server.properties /opt/kafka_2.13-2.6.0/config/server.properties
COPY zookeeper.properties /opt/kafka_2.13-2.6.0/config/zookeeper.properties

ENV PATH="/opt/kafka_2.13-2.6.0/bin:${PATH}"
ENV JAVA_HOME="/usr/lib/jvm/java-11-openjdk-amd64"

ENTRYPOINT ["/opt/kafka_2.13-2.6.0/bin/kafka-server-start.sh", "/opt/kafka_2.13-2.6.0/config/server.properties"]

