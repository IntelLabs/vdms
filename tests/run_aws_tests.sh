#!/bin/bash -e

# Command format:
# sh ./run_aws_tests.sh -u YOUR_MINIO_USERNAME -p YOUR_MINIO_PASSWORD [-n "RemoteConnectionTest.*"] [-s]
# You may use "-s" flag for stopping the testing process if any test fails
# You may specify a filter for running specific tests
# ex. sh ./run_aws_tests.sh -u YOUR_MINIO_USERNAME -p YOUR_MINIO_PASSWORD -n "ImageTest.*"
# ex. sh ./run_aws_tests.sh -u YOUR_MINIO_USERNAME -p YOUR_MINIO_PASSWORD -n "*" // It runs everything, due to the single match-everything * value.
# ex. sh ./run_aws_tests.sh -u YOUR_MINIO_USERNAME -p YOUR_MINIO_PASSWORD -n "FooTest.*" // Runs everything in test suite FooTest .
# ex. sh ./run_aws_tests.sh -u YOUR_MINIO_USERNAME -p YOUR_MINIO_PASSWORD -n "*Null*:*Constructor*" // Runs any test whose full name contains either "Null" or "Constructor" .
# ex. sh ./run_aws_tests.sh -u YOUR_MINIO_USERNAME -p YOUR_MINIO_PASSWORD -n "-*DeathTest.*" // Runs all non-death tests.
# ex. sh ./run_aws_tests.sh -u YOUR_MINIO_USERNAME -p YOUR_MINIO_PASSWORD -n "FooTest.*-FooTest.Bar" // Runs everything in test suite FooTest except FooTest.Bar.
# ex. sh ./run_aws_tests.sh -u YOUR_MINIO_USERNAME -p YOUR_MINIO_PASSWORD -n "FooTest.*:BarTest.*-FooTest.Bar:BarTest.Foo" // Runs everything in test suite FooTest except FooTest.Bar and everything in test suite BarTest except BarTest.Foo.

# Variable used for storing the process id for the minio server
py_minio_pid='UNKNOWN_PROCESS_ID'

function execute_commands() {
    username_was_set=false
    password_was_set=false
    api_port=${AWS_API_PORT}
    api_port_was_set=false
    testname_was_set=false
    stop_on_failure_was_set=false

    # Parse the arguments of the command
    while getopts u:p:a:n:s flag
    do
        case "${flag}" in
            u)
                username=${OPTARG}
                username_was_set=true
                ;;
            p)
                password=${OPTARG}
                password_was_set=true
                ;;
            a)
                api_port=${OPTARG}
                api_port_was_set=true
                ;;
            n)
                testname=${OPTARG}
                testname_was_set=true
                ;;
            s)
                stop_on_failure_was_set=true
                ;;
        esac
    done

    # Print variables
    echo "AWS Parameters used:"
    echo -e "\tapi_port:\t$api_port"
    echo -e "\tpassword:\t$password"
    echo -e "\tusername:\t$username"
    echo -e "\ttestname:\t$testname"

    if [ $username_was_set = false ] || [ $password_was_set = false ]; then
        echo 'Missing arguments for "run_aws_tests.sh" script'
        echo 'Usage: sh ./run_aws_tests.sh -u YOUR_MINIO_USERNAME -p YOUR_MINIO_PASSWORD [-n "RemoteConnectionTest.*"] [-s]'
        exit 1;
    fi

    # Using the flag "-s"
    # for specifying if google test has to stop the execution when
    # there is a failure in one of the tests
    stop_on_failure_value=""
    if [ "$stop_on_failure_was_set" = true ]; then
        stop_on_failure_value="--gtest_fail_fast"
        echo 'Using --gtest_fail_fast'
    fi

    # Using the flag "-n YOUR_TEST_NAME"
    # for specifying the GTest filter. In case that this flag is not specified
    # then it will use the default filter pattern
    test_filter="RemoteConnectionTest.*"
    if [ "$testname_was_set" = true ]; then
        test_filter=$testname
        echo 'Using test filter: '$test_filter
    fi

    # Kill current instances of minio
    echo 'Killing current instances of minio'
    pkill -9 minio || true
    sleep 2

    sh cleandbs.sh || true
    mkdir test_db_client || true
    mkdir dbs || true # necessary for Descriptors
    mkdir temp || true # necessary for Videos
    mkdir videos_tests || true
    mkdir backups || true

    #start the minio server
    ./../minio server ./../minio_files &
    py_minio_pid=$!

    sleep 2
    echo 'Creating buckets for the tests'
    # Create the minio-bucket for MinIO
    # by using the corresponding MinIO client which connects to the MinIO server
    # by using its username and password
    mc alias set myminio/ http://localhost:${api_port} $username $password
    mc mb myminio/minio-bucket

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

    echo "Killing the minio server"
    kill -9 $py_minio_pid || true

    echo 'Removing temporary files'
    rm -rf ../minio_files/ || true
    rm -rf test_db/ || true
    rm -rf test_db_aws/ || true
    rm -rf tdb/ || true
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
