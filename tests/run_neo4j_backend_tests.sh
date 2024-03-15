#!/bin/bash -e

# Setup the Neo4J container
echo "Initializing Neo4J Container"
export NEO_TEST_PORT=$1
docker run -d --name=4j_backend_test --env NEO4J_AUTH=neo4j/neo4jpass --publish=$1:7687 neo4j:5.17.0
echo "Sleeping for 30 seconds while neo4j initalizes"
sleep 30
# Issue functional tests using gtest framework
./../build/tests/unit_tests --gtest_filter=Neo4jBackendTest.*

# tear down the Neo4J Container
echo "Removing Neo4J Container"
docker kill 4j_backend_test
docker rm 4j_backend_test
