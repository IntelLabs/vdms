#!/bin/bash -e

sh cleandbs.sh || true
mkdir test_db_client
mkdir dbs  # necessary for Descriptors
mkdir temp # necessary for Videos
mkdir videos_tests
mkdir backups

# Start server for client test
./../build/vdms -cfg unit_tests/config-tests.json > tests_screen.log 2> tests_log.log &
cpp_unittest_pid=$!

./../build/vdms -cfg unit_tests/config-client-tests.json > tests_screen.log 2> tests_log.log &
client_test_pid=$!

echo 'not the vdms application - this file is needed for shared key' > vdms

echo 'Running C++ tests...'
./../build/tests/unit_tests \
    --gtest_filter=-ImageTest.CreateNameTDB:ImageTest.NoMetadata:VideoTest.CreateUnique:Descriptors_Add.add_1by1_and_search_1k

kill -9 $cpp_unittest_pid $client_test_pid
