#!/bin/bash -e

#######################################################################################################################
# RUN NEO4J TESTS
#   - Prior to running this script, neo4j container should be deployed using:
#         docker run --rm -d --env NEO4J_AUTH=$NEO4J_USER/$NEO4J_PASSWORD --publish=$NEO_TEST_PORT:7687 neo4j:5.17.0
#
# SYNTAX:
# ./run_neo4j_tests.sh -t TEST -a MINIO_API_PORT -c MINIO_CONSOLE_PORT -p YOUR_MINIO_PASSWORD -u YOUR_MINIO_USERNAME \
#                      -e NEO4J_ENDPOINT -v NEO_TEST_PORT -w NEO4J_PASSWORD -n NEO4J_USER
#######################################################################################################################
# USAGE
script_usage()
{
    cat <<EOF
    This script is used to run the Neo4J Tests.

    Usage: $0 [ -h ] [ -t TEST ] [ -a MINIO_API_PORT ] [ -c MINIO_CONSOLE_PORT ] [ -p YOUR_MINIO_PASSWORD ]
              [ -u YOUR_MINIO_USERNAME ] [ -e NEO4J_ENDPOINT ] [ -v NEO_TEST_PORT ] [ -w NEO4J_PASSWORD ]
              [ -n NEO4J_USER ]

    Options:
        -h or --help                Print this help message
        -a or --minio_api_port      API Port for Minio server
        -c or --minio_console_port  Console Port for Minio server
        -p or --minio_pswd          Password for Minio server
        -u or --minio_user          Username for Minio server
        -e or --neo4j_endpoint      Neo4j endpoint
        -v or --neo4j_port          Port for Neo4j container
        -w or --neo4j_pswd          Password for Neo4j container
        -n or --neo4j_user          Username for Neo4j container
        -t or --test_name           Name of test to run [OpsIOCoordinatorTest, Neo4jBackendTest, or Neo4JE2ETest]
EOF
}

LONG_LIST=(
    "help"
    "minio_api_port"
    "minio_console_port"
    "minio_pswd"
    "minio_user"
    "neo4j_endpoint"
    "neo4j_port"
    "neo4j_pswd"
    "neo4j_user"
    "test_name"
)

OPTS=$(getopt \
    --options "ha:c:p:u:e:v:w:n:t:" \
    --long help,minio_api_port:,minio_console_port:,minio_pswd:,minio_user:,neo4j_endpoint:,neo4j_port:,neo4j_pswd:,neo4j_user:,test_name: \
    --name "$(basename "$0")" \
    -- "$@"
)

eval set -- $OPTS

#######################################################################################################################
# FUNCTION DEFINITION

# Variable used for storing the process id for the minio server
py_minio_pid='UNKNOWN_PROCESS_ID'
cpp_unittest_pid='UNKNOWN_PROCESS_ID'

function execute_commands() {
    api_port=${AWS_API_PORT}
    api_port_was_set=false
    console_port=${AWS_CONSOLE_PORT}
    console_port_was_set=false
    minio_password=""
    minio_password_was_set=false
    minio_username=""
    minio_username_was_set=false
    neo4j_endpoint=""
    neo4j_endpoint_was_set=false
    neo4j_port=${NEO_TEST_PORT}
    neo4j_port_was_set=false
    neo4j_password=""
    neo4j_password_was_set=false
    neo4j_username=""
    neo4j_username_was_set=false
    test=""
    test_was_set=false

    while true; do
        case "$1" in
            -h | --help) script_usage; exit 0 ;;
            -a | --minio_api_port) shift; api_port=$1; api_port_was_set=true; shift ;;
            -c | --minio_console_port) shift; console_port=$1; console_port_was_set=true; shift ;;
            -p | --minio_pswd) shift; minio_password=$1; minio_password_was_set=true; shift ;;
            -u | --minio_user) shift; minio_username=$1; minio_username_was_set=true; shift ;;
            -e | --neo4j_endpoint) shift; neo4j_endpoint=$1; neo4j_endpoint_was_set=true; shift ;;
            -v | --neo4j_port) shift; neo4j_port=$1; neo4j_port_was_set=true; shift ;;
            -w | --neo4j_pswd) shift; neo4j_password=$1; neo4j_password_was_set=true; shift ;;
            -n | --neo4j_user) shift; neo4j_username=$1; neo4j_username_was_set=true; shift ;;
            -t | --test_name) shift; test=$1; test_was_set=true; shift ;;
            --) shift; break ;;
            *) script_usage; exit 0 ;;
        esac
    done

    if [ "$test_was_set" = false ]; then
        echo 'Missing test argument (-t) for "run_neo4j_tests.sh" script'
        script_usage
        exit 1;
    fi

    if [ "$test" = "OpsIOCoordinatorTest" ] || [ "$test" = "Neo4JE2ETest" ]; then
        #Test requires MinIO & Neo4J Container
        if [ "$minio_username_was_set" = false ] || [ "$minio_password_was_set" = false ] || [ "$neo4j_username_was_set" = false ] || [ "$neo4j_password_was_set" = false ] || [ "$neo4j_endpoint_was_set" = false ]; then
            echo 'Missing MinIO or Neo4j arguments for "run_neo4j_tests.sh" script'
            script_usage
            exit 1;
        fi

        if [ "$neo4j_endpoint_was_set" = false ]; then
            neo4j_endpoint=neo4j://localhost:$neo4j_port
        fi

        # Set minio bucket name
        if [ "$test" = "OpsIOCoordinatorTest" ]; then
            minio_name=opsio_tester
        elif [ "$test" = "Neo4JE2ETest" ]; then
            minio_name=e2e_tester
        fi

    elif [ "$test" = "Neo4jBackendTest" ]; then
        #Test requires Neo4J Container ONLY
        if [ "$neo4j_username_was_set" = false ] || [ "$neo4j_password_was_set" = false ] || [ "$neo4j_endpoint_was_set" = false ]; then
            echo 'Missing Neo4j arguments for "run_neo4j_tests.sh" script'
            script_usage
            exit 1;
        fi

    else
        echo 'Unknown test. Acceptable test names: OpsIOCoordinatorTest, Neo4jBackendTest, or Neo4JE2ETest'
        exit 1;
    fi

    # Export variables used in VDMS
    export NEO_TEST_PORT=$neo4j_port
    export NEO4J_USER=$neo4j_username
    export NEO4J_PASS=$neo4j_password
    export NEO4J_ENDPOINT=$neo4j_endpoint

    # Print variables
    echo "Neo4j Test Parameters used:"
    echo -e "\tapi_port:\t$api_port"
    echo -e "\tconsole_port:\t$console_port"
    echo -e "\tminio_password:\t$minio_password"
    echo -e "\tminio_username:\t$minio_username"
    echo -e "\tneo4j_endpoint:\t$neo4j_endpoint"
    echo -e "\tneo4j_port:\t$NEO_TEST_PORT"
    echo -e "\tneo4j_password:\t$NEO4J_PASS"
    echo -e "\tneo4j_username:\t$NEO4J_USER"
    echo -e "\ttest:\t$test\n\n"


    # Kill current instances of minio
    echo 'Killing current instances of minio'
    pkill -9 minio || true
    sleep 2

    # Clear other folders
    sh cleandbs.sh || true
    mkdir -p tests_output_dir
    mkdir -p tests_output_dir/test_db_client
    mkdir -p tests_output_dir/dbs  # necessary for Descriptors
    mkdir -p tests_output_dir/temp # necessary for Videos
    mkdir -p tests_output_dir/videos_tests
    mkdir -p tests_output_dir/backups
    mkdir -p tests_output_dir/neo4j_empty || true

    cp unit_tests/config-neo4j-e2e.json tests_output_dir/config-neo4j-e2e.json

    # For OpsIOCoordinatorTest tests
    cp unit_tests/config-aws-tests.json tests_output_dir/config-aws-tests.json

    if [ "$test" = "OpsIOCoordinatorTest" ] || [ "$test" = "Neo4JE2ETest" ]; then
        #start the minio server
        ./../minio server tests_output_dir/minio_files --address :${api_port} --console-address :${console_port} &
        py_minio_pid=$!

        sleep 2
        echo 'Creating MinIO buckets for the tests'
        # Create the minio-bucket for MinIO
        # by using the corresponding MinIO client which connects to the MinIO server
        # by using its username and password
        mc alias set ${minio_name}/ http://localhost:${api_port} $minio_username $minio_password
        mc mb ${minio_name}/minio-bucket
    fi

    if [ "$test" = "Neo4JE2ETest" ]; then
        echo "Starting VDMS Server"
        ./../build/vdms -cfg tests_output_dir/config-neo4j-e2e.json > tests_output_dir/neo4j-e2e_screen.log 2> tests_output_dir/neo4j-e2e_log.log &
	    cpp_unittest_pid=$!

        echo "Sleeping for 10 seconds while VDMS initializes..."
        sleep 10
    fi

    echo "Starting ${test}..."
    ./../build/tests/unit_tests --gtest_filter=${test}.*
    echo "Tests Complete!"
    exit 0
}

# Cleanup function to kill those processes which were started by the script
# Also it deletes those directories created by the script (or its tests)
function cleanup() {
    exit_value=$?

    if [ "$py_minio_pid" != 'UNKNOWN_PROCESS_ID' ]; then
        echo "Killing the minio server"
        kill -9 $py_minio_pid || true
    fi

    echo 'Removing temporary files'
    rm -rf tests_output_dir || true

    if [ "$test" = "Neo4JE2ETest" ]; then
        echo "Stopping the vdms server"
        kill -9 $cpp_unittest_pid || true
    fi

    exit $exit_value
}

#######################################################################################################################
# RUN SCRIPT

# Get the arguments sent to the script command
args=$@

# These traps call to cleanup() function when one those signals happen
trap cleanup EXIT
trap cleanup ERR
trap cleanup SIGINT

# Call to execute the script commands
execute_commands ${args}
