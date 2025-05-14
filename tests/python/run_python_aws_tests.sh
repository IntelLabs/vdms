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
# sh ./run_python_aws_tests.sh -u YOUR_MINIO_USERNAME -p YOUR_MINIO_PASSWORD [-n TEST_PATTERN_NAME]
# You may specify a filter for running specific tests
# ex. sh ./run_python_aws_tests.sh -u YOUR_MINIO_USERNAME -p YOUR_MINIO_PASSWORD -n test_module.TestClass.test_method
# ex. sh ./run_python_aws_tests.sh -u YOUR_MINIO_USERNAME -p YOUR_MINIO_PASSWORD -n "TestBoundingBox.TestBoundingBox.test_addBoundingBox"
# ex. sh ./run_python_aws_tests.sh -u YOUR_MINIO_USERNAME -p YOUR_MINIO_PASSWORD -n "TestBoundingBox.py"

# Variable used for storing the process id for the vdms server
py_unittest_pid='UNKNOWN_PROCESS_ID'
# Variable used for storing the process id for mTLS
py_tls_unittest_pid='UNKNOWN_PROCESS_ID'
# Variable used for storing the process id for the minio server
py_minio_pid='UNKNOWN_PROCESS_ID'

function execute_commands() {
    username_was_set=false
    password_was_set=false
    api_port=${AWS_API_PORT}
    api_port_was_set=false

    testname_was_set=false

    # Parse the arguments of the command
    while getopts u:p:a:n: flag
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
        esac
    done

    if [ $username_was_set = false ] || [ $password_was_set = false ]; then
        echo 'Missing arguments for "run_python_aws_tests.sh" script'
        echo 'Usage: sh ./run_python_aws_tests.sh -u YOUR_MINIO_USERNAME -p YOUR_MINIO_PASSWORD [-n TEST_PATTERN_NAME]'
        exit 1;
    fi

    # Using the flag "-n YOUR_TEST_NAME"
    # for specifying the unittest test filter. In case that this flag is not specified
    # then it will use the default filter pattern
    test_filter="discover --pattern=Test*.py"
    if [ "$testname_was_set" = true ]; then
        test_filter=$testname
        echo 'Using test filter: '$test_filter
    fi

    if [ "$api_port" = ' ' ] || [ "$api_port" = '' ]; then
        api_port=9000
        echo 'Default api_port'
    fi

    # Print variables
    echo "AWS Parameters used:"
    echo -e "\tapi_port:\t$api_port"
    echo -e "\tpassword:\t$password"
    echo -e "\tusername:\t$username"

    TEST_DIR=${PWD}
    base_dir=$(dirname $(dirname $PWD))
    client_path=${base_dir}/client/python
    export PYTHONPATH=$client_path:${PYTHONPATH}

    # Uncomment to re-generate queryMessage_pb2.py
    # protoc -I=${base_dir}/utils/src/protobuf --python_out=${client_path}/vdms ${base_dir}/utils/src/protobuf/queryMessage.proto

    cd ${TEST_DIR}

    # Stop current instances of minio
    echo 'Stopping current instances of minio'
    pkill -9 minio || true
    sleep 2

    echo 'Removing temporary files'
    rm -rf /tmp/tests_output_dir || true

    mkdir -p /tmp/tests_output_dir || true
    mkdir -p /tmp/tests_output_dir/test_db || true

    cp config-aws-tests.json /tmp/tests_output_dir/config-aws-tests.json
    cp config-tls-aws-tests.json /tmp/tests_output_dir/config-tls-aws-tests.json

    echo 'Starting vdms server'
    ./../../build/vdms -cfg /tmp/tests_output_dir/config-aws-tests.json > /tmp/tests_output_dir/screen.log 2> /tmp/tests_output_dir/log.log &
    py_unittest_pid=$!

    python3 ../tls_test/prep_certs.py
    ./../../build/vdms -cfg /tmp/tests_output_dir/config-tls-aws-tests.json > /tmp/tests_output_dir/screen-tls.log 2> /tmp/tests_output_dir/log-tls.log &
    py_tls_unittest_pid=$!

    sleep 1

    #start the minio server
    echo 'Starting minio server'
    ./../../minio server /tmp/tests_output_dir/minio_files --address :${api_port} &
    py_minio_pid=$!

    sleep 2
    echo 'Creating buckets for the tests'
    # Create the minio-bucket for MinIO
    # by using the corresponding MinIO client which connects to the MinIO server
    # by using its username and password
    mc alias set myminio/ http://localhost:${api_port} $username $password
    mc mb myminio/minio-bucket

    sleep 2

    # Starting the testing
    echo 'Starting the testing'
    echo 'Setting to True the VDMS_SKIP_REMOTE_PYTHON_TESTS env var'
    # There are some Python tests which have to be skipped as they are specific
    # for NON Remote tests, in order to do that the
    # 'VDMS_SKIP_REMOTE_PYTHON_TESTS' environment variable must be set to True
    export VDMS_SKIP_REMOTE_PYTHON_TESTS=True
    echo 'Running Python AWS S3 tests...'
    python3 -m coverage run --include="../../*" --omit="${base_dir}/client/python/vdms/queryMessage_pb2.py,../*" -m unittest $test_filter -v

    echo 'Finished'
    exit 0
}

# Cleanup function to stop those processes which were started by the script
# Also it deletes those directories created by the script (or its tests)
function cleanup() {
    exit_value=$?

    unset VDMS_SKIP_REMOTE_PYTHON_TESTS

    # Stopping vdms and minio processes after finishing the testing
    echo 'Stopping vdms, tls, and minio processes after finishing the testing'
    kill -9 $py_unittest_pid || true
    kill -9 $py_tls_unittest_pid || true
    kill -9 $py_minio_pid || true

    echo 'Removing temporary files'
    rm -rf /tmp/tests_output_dir/ || true
    rm -rf test_db/ || true
    rm -rf test_db_aws/ || true
    rm -rf test_db_tls/ || true

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
