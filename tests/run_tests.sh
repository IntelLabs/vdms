#!/bin/bash -e

# Variable used for storing the process id for the vdms server and client
cpp_unittest_pid='UNKNOWN_PROCESS_ID'
client_test_pid='UNKNOWN_PROCESS_ID'

function execute_commands() {
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
    python3 udf_server.py 5010 > ../tests_remote_screen.log 2> ../tests_remote_log.log &

    # Start UDF message queue for test
    cd ../udf_test
    python3 -m pip install -r requirements.txt
    python3 udf_local.py > ../tests_udf_screen.log 2> ../tests_udf_log.log &

    cd ..

    # Start server for client test
    ./../build/vdms -cfg unit_tests/config-tests.json > tests_screen.log 2> tests_log.log &
    cpp_unittest_pid=$!

    ./../build/vdms -cfg unit_tests/config-client-tests.json > tests_screen.log 2> tests_log.log &
    client_test_pid=$!

    echo 'not the vdms application - this file is needed for shared key' > vdms

    echo 'Running C++ tests...'
    ./../build/tests/unit_tests \
        --gtest_filter=-ImageTest.CreateNameTDB:ImageTest.NoMetadata:VideoTest.SyncRemoteWrite:VideoTest.UDFWrite:Descriptors_Add.add_1by1_and_search_1k:RemoteConnectionTest.*:Neo4jBackendTest.*
    echo 'Finished'
    exit 0
}

# Cleanup function to kill those processes which were started by the script
# Also it deletes those directories created by the script (or its tests)
function cleanup() {

    exit_value=$?
    
    echo "Killing the udf_server and udf_local"
    pkill -9 -f udf_server.py || true
    pkill -9 -f udf_local.py || true

    echo "Killing the vdms server and client"
    kill -9 $cpp_unittest_pid $client_test_pid  || true

    # Clean up
    echo 'Removing the temporary files created'
    sh ./cleandbs.sh || true
    
    exit $exit_value
}

# Get the arguments sent to the script command
args=$@

# These traps call to cleanup() function when one those signals happen
trap cleanup EXIT
trap cleanup ERR
trap cleanup SIGINT

# Call to execute the script commands
execute_commands ${args}
