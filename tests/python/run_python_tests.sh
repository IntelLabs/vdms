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
# sh ./run_python_tests.sh [-n TEST_PATTERN_NAME]
# You may specify a filter for running specific tests
# ex. sh ./run_python_tests.sh -n test_module.TestClass.test_method
# ex. sh ./run_python_tests.sh -n "TestBoundingBox.TestBoundingBox.test_addBoundingBox"
# ex. sh ./run_python_tests.sh -n "TestBoundingBox.py"

# Variable used for storing the process id for the vdms server
py_unittest_pid='UNKNOWN_PROCESS_ID'
py_tls_unittest_pid='UNKNOWN_PROCESS_ID'

function execute_commands() {

    testname_was_set=false

    # Parse the arguments of the command
    while getopts n: flag
    do
        case "${flag}" in
            n)
                testname=${OPTARG}
                testname_was_set=true
                ;;
        esac
    done

    # Using the flag "-n YOUR_TEST_NAME"
    # for specifying the unittest filter. In case that this flag is not specified
    # then it will use the default filter pattern
    test_filter="discover --pattern=Test*.py"
    if [ "$testname_was_set" = true ]; then
        test_filter=$testname
        echo 'Using test filter: '$test_filter
    fi

    TEST_DIR=${PWD}
    base_dir=$(dirname $(dirname $PWD))
    client_path=${base_dir}/client/python
    export PYTHONPATH=$client_path:${PYTHONPATH}

    # Uncomment to re-generate queryMessage_pb2.py
    # protoc -I=${base_dir}/utils/src/protobuf --python_out=${client_path}/vdms ${base_dir}/utils/src/protobuf/queryMessage.proto

    cd ${TEST_DIR}
    rm -rf test_db || true
    rm -rf log.log || true
    rm -rf screen.log || true
    mkdir -p test_db || true

    ./../../build/vdms -cfg config-tests.json > screen.log 2> log.log &
    py_unittest_pid=$!

    python3 ../tls_test/prep_certs.py
    ./../../build/vdms -cfg config-tls-tests.json > screen-tls.log 2> log-tls.log &
    py_tls_unittest_pid=$!

    sleep 1

    echo 'Running Python tests...'
    python3 -m coverage run --include="../../*" --omit="${base_dir}/client/python/vdms/queryMessage_pb2.py,../*" -m unittest $test_filter -v

    echo 'Finished'
    exit 0
}

# Cleanup function to kill those processes which were started by the script
# Also it deletes those directories created by the script (or its tests)
function cleanup() {
    exit_value=$?

    rm  -rf test_db || true
    rm -rf log.log || true
    rm -rf screen.log || true
    rm  -rf test_db_tls || true
    rm -rf log-tls.log || true
    rm -rf screen-tls.log || true
    kill -9 $py_unittest_pid || true
    kill -9 $py_tls_unittest_pid || true
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
