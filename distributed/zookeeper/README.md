need to call using the function
docker run -d -p56000:56000 zookeeper so the port is exposed to other applications on the same machine

when running the kafka, it is necessary to use the network names assinged in docker-compose not "loclahost"

 ./kafka-topics.sh --zookeeper localhost:56000 --topic first_topic --create --partitions 3 --replication-factor 2
