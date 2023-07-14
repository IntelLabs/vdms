#!/bin/bash -e

sh cleandbs.sh || true
mkdir test_db_client
mkdir dbs  # necessary for Descriptors
mkdir temp # necessary for Videos
mkdir videos_tests
mkdir backups

#start the minio server
./../minio server ./../minio_files &
py_minio_pid=$!

sleep 2

echo 'Running C++ tests...'
./../build/tests/unit_tests --gtest_filter=RemoteConnectionTest.*

kill -9 $py_minio_pid  || true
