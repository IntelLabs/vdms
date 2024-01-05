#!/bin/bash -e
#
# The MIT License
#
# @copyright Copyright (c) 2017 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction,
# including without limitation the rights to use, copy, modify,
# merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
# ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

# Command format:
# sh ./run_python_aws_tests.sh -u YOUR_MINIO_USERNAME -p YOUR_MINIO_PASSWORD

# Variable used for storing the process id for the vdms server
py_unittest_pid='UNKNOWN_PROCESS_ID'
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
        echo 'Missing arguments for "run_python_aws_tests.sh" script'
        echo 'Usage: sh ./run_python_aws_tests.sh -u YOUR_MINIO_USERNAME -p YOUR_MINIO_PASSWORD'
        exit 1;
    fi

    TEST_DIR=${PWD}
    base_dir=$(dirname $(dirname $PWD))
    client_path=${base_dir}/client/python
    export PYTHONPATH=$client_path:${PYTHONPATH}

    # Uncomment to re-generate queryMessage_pb2.py
    # protoc -I=${base_dir}/utils/src/protobuf --python_out=${client_path}/vdms ${base_dir}/utils/src/protobuf/queryMessage.proto

    cd ${TEST_DIR}

    # Kill current instances of minio
    echo 'Killing current instances of minio'
    pkill -9 minio || true
    sleep 2

    echo 'Removing temporary files'
    rm -rf ../../minio_files/ || true
    rm -rf test_db/ || true
    rm -rf test_db_aws/ || true

    rm  -rf test_db log.log screen.log
    mkdir -p test_db

    echo 'Starting vdms server'
    ./../../build/vdms -cfg config-aws-tests.json > screen.log 2> log.log &
    py_unittest_pid=$!

    sleep 1

    #start the minio server
    echo 'Starting minio server'
    ./../../minio server ./../../minio_files &
    py_minio_pid=$!

    sleep 2
    echo 'Creating buckets for the tests'
    # Create the minio-bucket for MinIO
    # by using the corresponding MinIO client which connects to the MinIO server
    # by using its username and password
    mc alias set myminio/ http://localhost:9000 $username $password
    mc mb myminio/minio-bucket

    sleep 2

    # Starting the testing
    echo 'Starting the testing'
    echo 'Running Python AWS S3 tests...'
    python3 -m coverage run --include="../../*" --omit="${base_dir}/client/python/vdms/queryMessage_pb2.py,../*" -m unittest discover --pattern=Test*.py -v
    echo 'Finished'
    exit 0
}

# Cleanup function to kill those processes which were started by the script
# Also it deletes those directories created by the script (or its tests)
function cleanup() {
    # Removing log files
    echo 'Removing log files'
    rm  -rf test_db log.log screen.log

    echo 'Removing temporary files'
    rm -rf ../../minio_files/ || true
    rm -rf test_db/ || true
    rm -rf test_db_aws/ || true

    # Killing vdms and minio processes after finishing the testing
    echo 'Killing vdms and minio processes after finishing the testing'
    kill -9 $py_unittest_pid $py_minio_pid || true
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
