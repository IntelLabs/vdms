#!/bin/bash -e

# Command format:
# sh ./run_aws_tests.sh -u YOUR_MINIO_USERNAME -p YOUR_MINIO_PASSWORD

# Variable used for storing the process id for the minio server
py_minio_pid='UNKNOWN_PROCESS_ID'

function execute_commands() {
    username_was_set=false
    password_was_set=false

    # Parse the arguments of the command
    while getopts u:p: flag
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
        esac
    done

    if [ $username_was_set = false ] || [ $password_was_set = false ]; then
        echo 'Missing arguments for "run_aws_tests.sh" script'
        echo 'Usage: sh ./run_aws_tests.sh -u YOUR_MINIO_USERNAME -p YOUR_MINIO_PASSWORD'
        exit 1;
    fi

    # Kill current instances of minio
    echo 'Killing current instances of minio'
    pkill -9 minio || true
    sleep 2

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
    echo 'Creating buckets for the tests'
    # Create the minio-bucket for MinIO
    # by using the corresponding MinIO client which connects to the MinIO server
    # by using its username and password
    mc alias set myminio/ http://localhost:9000 $username $password
    mc mb myminio/minio-bucket

    echo 'Running C++ tests...'
    ./../build/tests/unit_tests --gtest_filter=RemoteConnectionTest.*

    echo 'Finished'
    exit 0
}

# Cleanup function to kill those processes which were started by the script
# Also it deletes those directories created by the script (or its tests)
function cleanup() {
    echo "Killing the minio server"
    kill -9 $py_minio_pid || true

    echo 'Removing temporary files'
    rm -rf ../minio_files/ || true
    rm -rf test_db/ || true
    rm -rf test_db_aws/ || true
    rm -rf tdb/ || true
    exit 0
}

# Get the arguments sent to the script command
args=$@

# These traps call to cleanup() function when one those signals happen
trap cleanup EXIT
trap cleanup ERR
trap cleanup SIGINT

# Call to execute the script commands
execute_commands ${args}
