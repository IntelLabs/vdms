# VDMS Test Script

The `run_all_tests.py` script oversees the distinct types of tests located at the `tests` directory of the VDMS repo.

It manages some signals so in case of any segmentation fault or any error, then this script will delete all the temporary files created (except when the -k flag is specified for keeping the temporary files when the testing finished) and it will kill all the processes started.

## Architecture of the run_all_tests.py script file

### Classes

The classes found in this script file are:

**TestingArgs:** This class is in charge of storing the value of most of the arguments set by command line or from the JSON config file.

**TestingParser:** This class provides a parser for command-line arguments used in testing.  It uses the argparse library to define and parse the arguments.

### **Base class**

**AbstractTest:** This class defines the interface for test cases, including methods for running tests, validating arguments, and filling default arguments. All the abstract methods must be implemented by subclasses:

* run(self, testingArgs: TestingArgs): This abstract method is in charge of running the different applications needed by the specific type of test.

* validate_arguments(self, testingArgs: TestingArgs, parser: argparse.ArgumentParser): This abstract method must be implemented by each derived class, this method must validate the arguments received from the command line which arguments are required by each type of test.

* fill_default_arguments(self, testingArgs: TestingArgs): Due to each type of test requires mandatory information to execute the tests correctly, if that information does not come from the command line, then this script assumes that the user wants to use the default values provided by this script file to set the values for those missing arguments. Therefore, this method must be focused on filing out the values of the missing arguments.

### **Derived classes**

* **Neo4jTest:** This class provides methods to validate Neo4j values and run Neo4j tests.  It extends the AbstractTest class.

The Neo4jTest class implements the logic which exists in the Bash script called “tests/run_neo4j_tests.sh”

**Note:**
The integration of the Neo4JHandlerTest tests is pending to be added to the methods of this class.

* **NonRemoteTest:** This class provides methods to set up requirements for the remote UDF server and run non-remote tests. It extends the AbstractTest class.

The NonRemoteTest class implements the logic which exists in the Bash script called “tests/run_tests.sh”.

* **NonRemotePythonTest:** This class provides methods to set up the environment and run non-remote Python tests. It extends the AbstractTest class.

The NonRemotePythonTest class implements the logic which exists in the Bash script called “tests/python/run_python_tests.sh”.

* **RemoteTest:** This class provides methods to set up the environment and run remote C++ tests. It extends the AbstractTest class.

The RemoteTest class implements the logic which exists in the Bash script called “tests/run_aws_tests.sh”.

* **RemotePythonTest:** This class provides methods to set up the environment and run remote Python tests. It extends the AbstractTest class.

The RemotePythonTest class implements the logic which exists in the Bash script called “tests/python/run_python_aws_tests.sh”.

**Note**

If you need to create a new type of test such as integration tests, then you need to create a new class derived from the AbstractClass and implement at least the mandatory abstract methods.

If you just need to add new arguments to the current classes, then please update the code in the mandatory abstract methods, to correctly manage those new arguments.

Also add the new arguments in the TestingArgs class and add the corresponding parsing of those new arguments in the TestingParser class.


### Constant values

These are some constant values defined at the beginning of the script file:

The value of this variable is the directory where the run_all_tests.py file is located at

`DEFAULT_CURRENT_DIR = os.path.realpath(os.path.dirname(__file__))`

The root of the repository

`DEFAULT_DIR_REPO = os.path.dirname(DEFAULT_CURRENT_DIR)`

The path where the VDMS binary is located at

`DEFAULT_VDMS_APP_PATH = DEFAULT_DIR_REPO + "/build/vdms"`

The default path where the MinIO binary is located at

`DEFAULT_MINIO_PATH = DEFAULT_DIR_REPO + "/minio"`

The alias name for the MinIO server (used by the MinIO client mc)

`DEFAULT_MINIO_ALIAS_NAME = "myminio"`

The default port number where the MinIO server runs

`DEFAULT_MINIO_PORT = 9000`

The default name of the directory where MinIO stores the temporary files

`DEFAULT_MINIO_TMP_DIR = "minio_files"`

The default port number to connect to MinIO server via console

`DEFAULT_MINIO_CONSOLE_PORT = 9001`

The flag used by Googletest stops running the rest of the  tests when one of the tests fails

`STOP_ON_FAILURE_FLAG = "--gtest_fail_fast"`

The default port number for the Neo4j server

`DEFAULT_NEO_TEST_PORT = 7687`

The default endpoint for Neo4j

`DEFAULT_NEO_TEST_ENDPOINT = f"neo4j://neo4j:{str(DEFAULT_NEO_TEST_PORT)}"`

The default path where the binary used by Googletest is located at

`DEFAULT_GOOGLETEST_PATH = DEFAULT_DIR_REPO + "/build/tests/unit_tests"`

The name of the logfiles used by unit_tests binary (Googletest) for logging any event (also it is displayed on console)

`DEFAULT_TESTS_STDERR_FILENAME = "tests_stderr_log.log"`

`DEFAULT_TESTS_STDOUT_FILENAME = "tests_stdout_log.log"`

The name of the logfiles used by udf_local.py file for logging any event

`DEFAULT_UDF_LOCAL_STDERR_FILENAME = "udf_local_stderr_log.log"`

`DEFAULT_UDF_LOCAL_STDOUT_FILENAME = "udf_local_stdout_log.log"`

The name of the logfiles used by udf_server.py file for logging any event

`DEFAULT_UDF_SERVER_STDERR_FILENAME = "udf_server_stderr_log.log"`

`DEFAULT_UDF_SERVER_STDOUT_FILENAME = "udf_server_stdout_log.log"`

The name of the logfiles used by TLS script files for logging any event

`DEFAULT_TLS_STDERR_FILENAME = "tls_stderr_log.log"`

`DEFAULT_TLS_STDOUT_FILENAME = "tls_stdout_log.log"`

The name of the logfiles used by MinIO server for logging any event

`DEFAULT_MINIO_STDERR_FILENAME = "minio_stderr_log.log"`

`DEFAULT_MINIO_STDOUT_FILENAME = "minio_stdout_log.log"`

The name of the logfiles used by VDMS server for logging any event

`DEFAULT_VDMS_STDERR_FILENAME = "vdms_stderr_log.log"`

`DEFAULT_VDMS_STDOUT_FILENAME = "vdms_stdout_log.log"`

These are the types of tests available and which can be run by running the run_all_tests.py script

`TYPE_OF_TESTS_AVAILABLE = ["ut", "ru", "pt", "rp", "neo"]`

where:

| Option | Description |
| ------ | ----------- |
| `ut` | C++ unit tests located at tests/unit_tests directory |
| `ru` | Remote C++ unit tests located at tests/unit_tests directory |
| `pt` | Python tests located at tests/python directory |
| `rp` | Remote Python tests located at tests/python directory |
| `neo` | C++ unit tests located at tests/unit_tests directory |

The types of tests that Googletest can run

`GOOGLETEST_TYPE_OF_TESTS = ["ut", "ru", "neo"]`

The default path to the temporary directory used by the tests

`DEFAULT_TMP_DIR = "/tmp/tests_output_dir"`

Name of the directory in the repo where the tests are located at

`TESTS_DIRNAME = "tests"`

Pattern used by Googletest to run all the tests related to Neo4j and OpsIOCoordinator

`DEFAULT_NEO4J_OPSIO_TEST_FILTER = "OpsIOCoordinatorTest.*"`

Path to the configuration files used by the Neo4 OpsIOCoordinator tests

`DEFAULT_NEO4J_OPSIO_CONFIG_FILES = ["unit_tests/config-aws-tests.json"]`

Pattern used by Googletest to run all the tests related to the end-to-end tests of Neo4j

`DEFAULT_NEO4J_E2E_TEST_FILTER = "”`

`DEFAULT_NEO4J_E2E_CONFIG_FILES = ["unit_tests/config-neo4j-e2e.json"]`

Name of the buckets created by MinIO according to the type of Neo4j tests

`NEO4J_OPS_IO_TEST_TYPE = "OPS_IO"`

`NEO4J_E2E_TEST_TYPE = "E2E"`

`NEO4J_BACKEND_TEST_TYPE = "BACKEND"`

Regular expression to match any OpsIOCoordinatorTest test suite

`NEO4J_OPS_IO_REGEX = r"^OpsIOCoordinatorTest\.|^'OpsIOCoordinatorTest\.[^']+'"`

Regular expression to match any Neo4JE2ETest test suite

`NEO4J_E2E_REGEX = r"^Neo4JE2ETest\.|^'Neo4JE2ETest\.[^']+'"`

Regular expression to match any Neo4jBackendTest test suite

`NEO4J_BACKEND_REGEX = r"^Neo4jBackendTest\.|^'Neo4jBackendTest\.[^']+'"`

Path to the configuration files used by the local Python tests

`DEFAULT_NON_REMOTE_PYTHON_CONFIG_FILES = [`
    `"python/config-tests.json",`
    `"python/config-tls-tests.json",`
`]`

Pattern used by the unittest module used by Python for running all the tests which name starts with “Test” and they are located at the tests/python directory

`DEFAULT_PYTHON_TEST_FILTER = "discover -s ./python/ --pattern=Test*.py"`

Pattern used by the unittest module used by Python to run all the local Python tests

`DEFAULT_NON_REMOTE_PYTHON_TEST_FILTER = DEFAULT_PYTHON_TEST_FILTER`

Pattern used by Googletest to run all the local C++ unit tests (it executes all the tests except the specified in that list)

`DEFAULT_NON_REMOTE_UNIT_TEST_FILTER = "-RemoteConnectionTest.*:Neo4jBackendTest.*:OpsIOCoordinatorTest.*:Neo4JE2ETest.*:Neo4JHandlerTest.*"`

Path to the configuration files used by the local C++ unit tests

`DEFAULT_NON_REMOTE_UNIT_TEST_CONFIG_FILES = [`
    `"unit_tests/config-tests.json",`
    `"unit_tests/config-client-tests.json",`
`]`

Pattern used by Googletest to run all the remote C++ unit tests (it executes all the tests from the RemoteConnectionTest test suite)

`DEFAULT_REMOTE_UNIT_TEST_FILTER = "RemoteConnectionTest.*"`

Path to the configuration files used by the remote C++ unit tests

`DEFAULT_REMOTE_UNIT_TEST_CONFIG_FILES = ["unit_tests/config-aws-tests.json"]`

Pattern used by the unittest module used by Python to run all the remote Python tests

`DEFAULT_REMOTE_PYTHON_TEST_FILTER = DEFAULT_PYTHON_TEST_FILTER`

Path to the configuration files used by the remote Python tests

`DEFAULT_REMOTE_PYTHON_CONFIG_FILES = [`
    `"python/config-aws-tests.json",`
    `"python/config-tls-aws-tests.json",`
`]`
<br>


## Running the tests

The basic commands for running all the tests are the following:

* For running all the local C++ unit tests: `python3 run_all_tests.py -t ut`

* For running all the remote C++ unit tests: `python3 run_all_tests.py -t ut -u YOUR_MINIO_ROOT_USER -p YOUR_MINIO_ROOT_PASSWORD`

**Note:**

> YOUR_MINIO_ROOT_USER corresponds to your MinIO username

> YOUR_MINIO_ROOT_PASSWORD corresponds to your MinIO password.

* For running all the local python tests: `python3 run_all_tests.py -t pt`

* For running all the remote Python tests: `python3 run_all_tests.py -t rp -u YOUR_MINIO_ROOT_USER -p YOUR_MINIO_ROOT_PASSWORD`

**Note:**

> YOUR_MINIO_ROOT_USER corresponds to your MinIO username

> YOUR_MINIO_ROOT_PASSWORD corresponds to your MinIO password.

* For running the Neo4j tests: `python3 run_all_tests.py -t neo -n 'Neo4JE2ETest.*' --minio_port 9000 --minio_console_port 9001 -u YOUR_MINIO_ROOT_USER -p YOUR_MINIO_ROOT_PASSWORD --neo4j_username YOUR_NEO4J_USERNAME --neo4j_password YOUR_NEO4J_PASSWORD --neo4j_port 7687 --neo4j_endpoint neo4j://localhost:7687`

**Note:**

> YOUR_MINIO_ROOT_USER corresponds to your MinIO username.

> YOUR_MINIO_ROOT_PASSWORD corresponds to your MinIO password.

> YOUR_NEO4J_USERNAME corresponds to your Neo4j username.

> YOUR_MINIO_ROOT_PASSWORD corresponds to your Neo4j password.


You can find an explanation of the rest of arguments accepted by the run_all_tests.py in [VDMS Test Suite Overview](VDMS-Test-Suite.md)

