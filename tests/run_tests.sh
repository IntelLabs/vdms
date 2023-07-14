#!/bin/bash -e

sh cleandbs.sh || true
mkdir test_db_client
mkdir dbs  # necessary for Descriptors
mkdir temp # necessary for Videos
mkdir videos_tests
mkdir backups

# Stop UDF Queue and Remote Server if already running
pkill -9 -f udf_server.py || true
pkill -9 -f udf_local.py || true

# Start remote server for test
cd remote_function_test
python3 -m pip install -r requirements.txt
python3 udf_server.py 5010 > ../tests_screen.log 2> ../tests_log.log &

# Start UDF message queue for test
cd ../udf_test
python3 -m pip install -r requirements.txt
python3 udf_local.py > ../tests_screen.log 2> ../tests_log.log &

cd ..

# Start server for client test
./../build/vdms -cfg unit_tests/config-tests.json > tests_screen.log 2> tests_log.log &
cpp_unittest_pid=$!

./../build/vdms -cfg unit_tests/config-client-tests.json > tests_screen.log 2> tests_log.log &
client_test_pid=$!

echo 'not the vdms application - this file is needed for shared key' > vdms

echo 'Running C++ tests...'
./../build/tests/unit_tests \
    --gtest_filter=-ImageTest.CreateNameTDB:ImageTest.NoMetadata:VideoTest.CreateUnique:Descriptors_Add.add_1by1_and_search_1k:RemoteConnectionTest.*

pkill -9 -f udf_server.py
pkill -9 -f udf_local.py

kill -9 $cpp_unittest_pid $client_test_pid  || true
