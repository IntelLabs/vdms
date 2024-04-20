#!/bin/bash -e

# Command format:
# sh ./run_tests.sh [-n TEST_PATTERN_NAME] [-s]
# You may use "-s" flag for stopping the testing process if any test fails
# You may specify a filter for running specific tests
# ex. sh ./run_tests.sh -n "ImageTest.*"
# ex. sh ./run_tests.sh -n "*" // It runs everything, due to the single match-everything * value.
# ex. sh ./run_tests.sh -n "FooTest.*" // Runs everything in test suite FooTest .
# ex. sh ./run_tests.sh -n "*Null*:*Constructor*" // Runs any test whose full name contains either "Null" or "Constructor" .
# ex. sh ./run_tests.sh -n "-*DeathTest.*" // Runs all non-death tests.
# ex. sh ./run_tests.sh -n "FooTest.*-FooTest.Bar" // Runs everything in test suite FooTest except FooTest.Bar.
# ex. sh ./run_tests.sh -n "FooTest.*:BarTest.*-FooTest.Bar:BarTest.Foo" // Runs everything in test suite FooTest except FooTest.Bar and everything in test suite BarTest except BarTest.Foo.

# Variable used for storing the process id for the vdms server and client
cpp_unittest_pid='UNKNOWN_PROCESS_ID'
client_test_pid='UNKNOWN_PROCESS_ID'

function execute_commands() {
    testname_was_set=false
    stop_on_failure_was_set=false

    # Parse the arguments of the command
    while getopts n:s flag
    do
        case "${flag}" in
            n)
                testname=${OPTARG}
                testname_was_set=true
                ;;
            s)
                stop_on_failure_was_set=true
                ;;
        esac
    done

    # Using the flag "-n YOUR_TEST_NAME"
    # for specifying the GTest filter. In case that this flag is not specified
    # then it will use the default filter pattern
    test_filter="-RemoteConnectionTest.*:Neo4jBackendTest.*:OpsIOCoordinatorTest.*:Neo4JE2ETest.*"
    if [ "$testname_was_set" = true ]; then
        test_filter=$testname
        echo 'Using test filter: '$test_filter
    fi

    # Using the flag "-s"
    # for specifying if google test has to stop the execution when
    # there is a failure in one of the tests
    stop_on_failure_value=""
    if [ "$stop_on_failure_was_set" = true ]; then
        stop_on_failure_value="--gtest_fail_fast"
        echo 'Using --gtest_fail_fast'
    fi

    sh cleandbs.sh || true
    mkdir test_db_client
    mkdir dbs || true # necessary for Descriptors
    mkdir temp || true # necessary for Videos
    mkdir videos_tests || true
    mkdir backups || true

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
    sleep 3 # Wait for VMDS server to be initialized

    echo 'Running C++ tests...'
    ./../build/tests/unit_tests \
        --gtest_filter=$test_filter \
        $stop_on_failure_value
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
    kill -9 $cpp_unittest_pid || true
    kill -9 $client_test_pid || true

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
