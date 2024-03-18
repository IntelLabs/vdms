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

# Variable used for storing the process id for the vdms server
py_unittest_pid='UNKNOWN_PROCESS_ID'

function execute_commands() {
    TEST_DIR=${PWD}
    base_dir=$(dirname $(dirname $PWD))
    client_path=${base_dir}/client/python
    export PYTHONPATH=$client_path:${PYTHONPATH}

    # Uncomment to re-generate queryMessage_pb2.py
    # protoc -I=${base_dir}/utils/src/protobuf --python_out=${client_path}/vdms ${base_dir}/utils/src/protobuf/queryMessage.proto

    cd ${TEST_DIR}
    rm  -rf test_db log.log screen.log
    mkdir -p test_db

    ./../../build/vdms -cfg config-tests.json > screen.log 2> log.log &
    py_unittest_pid=$!

    sleep 1

    echo 'Running Python tests...'
    python3 -m coverage run --include="../../*" --omit="${base_dir}/client/python/vdms/queryMessage_pb2.py,../*" -m unittest discover --pattern=Test*.py -v

    echo 'Finished'
    exit 0
}

# Cleanup function to kill those processes which were started by the script
# Also it deletes those directories created by the script (or its tests)
function cleanup() {
    exit_value=$?

    rm  -rf test_db log.log screen.log
    kill -9 $py_unittest_pid || true
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
