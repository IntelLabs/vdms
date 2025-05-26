# VDMS Test Script

The `run_all_tests.py` script oversees the distinct types of tests located at the `tests` directory of the VDMS repo.

It manages some signals so in case of any segmentation fault or any error, then this script will delete all the temporary files created (except when the `-k` flag is specified for keeping the temporary files when the testing finished) and it will kill all the processes started.


## Architecture Script

### Classes

The classes found in this script file are:

* **TestingArgs:** This class is in charge of storing the value of most of the arguments set by command line or from the JSON config file.

* **TestingParser:** This class provides a parser for command-line arguments used in testing.  It uses the argparse library to define and parse the arguments.


### Base class

**AbstractTest:** This class defines the interface for test cases, including methods for running tests, validating arguments, and filling default arguments. All the abstract methods must be implemented by subclasses:

* `run(self, testingArgs: TestingArgs)`: This abstract method is in charge of running the different applications needed by the specific type of test.

* `validate_arguments(self, testingArgs: TestingArgs, parser: argparse.ArgumentParser)`: This abstract method must be implemented by each derived class, this method must validate the arguments received from the command line which arguments are required by each type of test.

* `fill_default_arguments(self, testingArgs: TestingArgs)`: Due to each type of test requires mandatory information to execute the tests correctly, if that information does not come from the command line, then this script assumes that the user wants to use the default values provided by this script file to set the values for those missing arguments. Therefore, this method must be focused on filing out the values of the missing arguments.


### Derived classes

* **Neo4jTest:** This class provides methods to validate Neo4j values and run Neo4j tests.  It extends the `AbstractTest` class. The `Neo4jTest` class implements the logic which exists in the Bash script called `tests/run_neo4j_tests.sh`.<br>***Note:*** The integration of the `Neo4JHandlerTest` tests is pending to be added to the methods of this class.

* **NonRemoteTest:** This class provides methods to set up requirements for the remote UDF server and run non-remote tests. It extends the `AbstractTest` class. The `NonRemoteTest` class implements the logic which exists in the Bash script called `tests/run_tests.sh`.

* **NonRemotePythonTest:** This class provides methods to set up the environment and run non-remote Python tests. It extends the `AbstractTest` class. The `NonRemotePythonTest` class implements the logic which exists in the Bash script called `tests/python/run_python_tests.sh`.

* **RemoteTest:** This class provides methods to set up the environment and run remote C++ tests. It extends the `AbstractTest` class. The `RemoteTest` class implements the logic which exists in the Bash script called `tests/run_aws_tests.sh`.

* **RemotePythonTest:** This class provides methods to set up the environment and run remote Python tests. It extends the `AbstractTest` class. The `RemotePythonTest` class implements the logic which exists in the Bash script called `tests/python/run_python_aws_tests.sh`.

***Notes:***

* If you need to create a new type of test such as integration tests, then you need to create a new class derived from the `AbstractClass` and implement at least the mandatory abstract methods.
* If you just need to add new arguments to the current classes, then please update the code in the mandatory abstract methods, to correctly manage those new arguments.
* Also add the new arguments in the `TestingArgs` class and add the corresponding parsing of those new arguments in the `TestingParser` class.


### Constant values

These are some constant values defined at the beginning of the script file:

| Variable | Description | Default Value |
| -------- | ----------- | ------------- |
| `DEFAULT_CURRENT_DIR` | The directory where the `run_all_tests.py` file is located | `os.path.realpath(os.path.dirname(__file__))` |
| `DEFAULT_DIR_REPO` | The root of the repository | `os.path.dirname(DEFAULT_CURRENT_DIR)` |
| `DEFAULT_VDMS_APP_PATH` | The path where the VDMS binary is located | `DEFAULT_DIR_REPO + "/build/vdms"` |
| `DEFAULT_MINIO_PATH` | The default path where the MinIO binary is located | `DEFAULT_DIR_REPO + "/minio"` |
| `DEFAULT_MINIO_ALIAS_NAME` | The alias name for the MinIO server (used by the MinIO client mc) | `"myminio"` |
| `DEFAULT_MINIO_PORT` | The default port number where the MinIO server runs | `9000` |
| `DEFAULT_MINIO_TMP_DIR` | The default name of the directory where MinIO stores the temporary files | `"minio_files"` |
| `DEFAULT_MINIO_CONSOLE_PORT` | The default port number to connect to MinIO server via console | `9001` |
| `STOP_ON_FAILURE_FLAG` | The flag used by Googletest stops running the rest of the  tests when one of the tests fails | `"--gtest_fail_fast"` |
| `DEFAULT_NEO_TEST_PORT` | The default port number for the Neo4j server | `7687` |
| `DEFAULT_NEO_TEST_ENDPOINT` | The default endpoint for Neo4j | `f"neo4j://neo4j:{str(DEFAULT_NEO_TEST_PORT)}"` |
| `DEFAULT_GOOGLETEST_PATH` | The default path where the binary used by Googletest is located | `DEFAULT_DIR_REPO + "/build/tests/unit_tests"` |
| `DEFAULT_TESTS_STDERR_FILENAME`<br>`DEFAULT_TESTS_STDOUT_FILENAME` | The name of the logfiles used by unit_tests binary (Googletest) for logging any event (also it is displayed on console) | `"tests_stderr_log.log"`<br>`"tests_stdout_log.log"` |
| `DEFAULT_UDF_LOCAL_STDERR_FILENAME`<br>`DEFAULT_UDF_LOCAL_STDOUT_FILENAME` | The name of the logfiles used by udf_local.py file for logging any event | `"udf_local_stderr_log.log"`<br>`"udf_local_stdout_log.log"` |
| `DEFAULT_UDF_SERVER_STDERR_FILENAME`<br>`DEFAULT_UDF_SERVER_STDOUT_FILENAME` | The name of the logfiles used by udf_server.py file for logging any event | `"udf_server_stderr_log.log"`<br>`"udf_server_stdout_log.log"` |
| `DEFAULT_TLS_STDERR_FILENAME`<br>`DEFAULT_TLS_STDOUT_FILENAME` | The name of the logfiles used by TLS script files for logging any event | `"tls_stderr_log.log"`<br>`"tls_stdout_log.log"` |
| `DEFAULT_MINIO_STDERR_FILENAME`<br>`DEFAULT_MINIO_STDOUT_FILENAME` | The name of the logfiles used by MinIO server for logging any event | `"minio_stderr_log.log"`<br>`"minio_stdout_log.log"` |
| `DEFAULT_VDMS_STDERR_FILENAME`<br>`DEFAULT_VDMS_STDOUT_FILENAME` | The name of the logfiles used by VDMS server for logging any event | `"vdms_stderr_log.log"`<br>`"vdms_stdout_log.log"` |
| `TYPE_OF_TESTS_AVAILABLE` | These are the types of tests available and which can be run by running the run_all_tests.py script | `["ut", "ru", "pt", "rp", "neo"]` where:<br> * `ut`: C++ unit tests located at tests/unit_tests directory<br> * `ru`: Remote C++ unit tests located at tests/unit_tests directory<br> * `pt`: Python tests located at tests/python directory<br> * `rp`: Remote Python tests located at tests/python directory<br> * `neo`: C++ unit tests located at tests/unit_tests directory |
| `GOOGLETEST_TYPE_OF_TESTS` | The types of tests that Googletest can run | `["ut", "ru", "neo"]` |
| `DEFAULT_TMP_DIR` | The default path to the temporary directory used by the tests | `"/tmp/tests_output_dir"` |
| `TESTS_DIRNAME` | Name of the directory in the repo where the tests are located | `"tests"` |
| `DEFAULT_NEO4J_OPSIO_TEST_FILTER` | Pattern used by Googletest to run all the tests related to Neo4j and OpsIOCoordinator | `"OpsIOCoordinatorTest.*"` |
| `DEFAULT_NEO4J_OPSIO_CONFIG_FILES` | Path to the configuration files used by the Neo4 OpsIOCoordinator tests | `["unit_tests/config-aws-tests.json"]` |
| `DEFAULT_NEO4J_E2E_TEST_FILTER`<br>`DEFAULT_NEO4J_E2E_CONFIG_FILES` | Pattern used by Googletest to run all the tests related to the end-to-end tests of Neo4j | `"”`<br>`["unit_tests/config-neo4j-e2e.json"]` |
| `NEO4J_OPS_IO_TEST_TYPE`<br>`NEO4J_E2E_TEST_TYPE`<br>`NEO4J_BACKEND_TEST_TYPE` | Name of the buckets created by MinIO according to the type of Neo4j tests | `"OPS_IO"`<br>`"E2E"`<br>`"BACKEND"` |
| `NEO4J_OPS_IO_REGEX` | Regular expression to match any OpsIOCoordinatorTest test suite | `r"^OpsIOCoordinatorTest\.|^'OpsIOCoordinatorTest\.[^']+'"` |
| `NEO4J_E2E_REGEX` | Regular expression to match any Neo4JE2ETest test suite | `r"^Neo4JE2ETest\.|^'Neo4JE2ETest\.[^']+'"` |
| `NEO4J_BACKEND_REGEX` | Regular expression to match any Neo4jBackendTest test suite | `r"^Neo4jBackendTest\.|^'Neo4jBackendTest\.[^']+'"` |
| `DEFAULT_NON_REMOTE_PYTHON_CONFIG_FILES` | Path to the configuration files used by the local Python tests | `["python/config-tests.json","python/config-tls-tests.json"]` |
| `DEFAULT_PYTHON_TEST_FILTER` | Pattern used by the unittest module used by Python for running all the tests which name starts with “Test” and they are located at the tests/python directory | `"discover -s ./python/ --pattern=Test*.py"` |
| `DEFAULT_NON_REMOTE_PYTHON_TEST_FILTER` | Pattern used by the unittest module used by Python to run all the local Python tests | `DEFAULT_PYTHON_TEST_FILTER` |
| `DEFAULT_NON_REMOTE_UNIT_TEST_FILTER` | Pattern used by Googletest to run all the local C++ unit tests (it executes all the tests except the specified in that list) | `"-RemoteConnectionTest.*:Neo4jBackendTest.*:OpsIOCoordinatorTest.*:Neo4JE2ETest.*:Neo4JHandlerTest.*"` |
| `DEFAULT_NON_REMOTE_UNIT_TEST_CONFIG_FILES` | Path to the configuration files used by the local C++ unit tests | `["unit_tests/config-tests.json","unit_tests/config-client-tests.json"]` |
| `DEFAULT_REMOTE_UNIT_TEST_FILTER` | Pattern used by Googletest to run all the remote C++ unit tests (it executes all the tests from the RemoteConnectionTest test suite) | `"RemoteConnectionTest.*"` |
| `DEFAULT_REMOTE_UNIT_TEST_CONFIG_FILES` | Path to the configuration files used by the remote C++ unit tests | `["unit_tests/config-aws-tests.json"]` |
| `DEFAULT_REMOTE_PYTHON_TEST_FILTER` | Pattern used by the unittest module used by Python to run all the remote Python tests | `DEFAULT_PYTHON_TEST_FILTER` |
| `DEFAULT_REMOTE_PYTHON_CONFIG_FILES` | Path to the configuration files used by the remote Python tests | `["python/config-aws-tests.json","python/config-tls-aws-tests.json"]` |

<br>

## Running Tests

The basic commands for running all the tests are the following:

* The local C++ unit tests: `python3 run_all_tests.py -t ut`

* The remote C++ unit tests: `python3 run_all_tests.py -t ut -u YOUR_MINIO_ROOT_USER -p YOUR_MINIO_ROOT_PASSWORD`

* The local python tests: `python3 run_all_tests.py -t pt`

* The remote Python tests: `python3 run_all_tests.py -t rp -u YOUR_MINIO_ROOT_USER -p YOUR_MINIO_ROOT_PASSWORD`

* For running the Neo4j tests: `python3 run_all_tests.py -t neo -n 'Neo4JE2ETest.*' --minio_port 9000 --minio_console_port 9001 -u YOUR_MINIO_ROOT_USER -p YOUR_MINIO_ROOT_PASSWORD --neo4j_username YOUR_NEO4J_USERNAME --neo4j_password YOUR_NEO4J_PASSWORD --neo4j_port 7687 --neo4j_endpoint neo4j://localhost:7687`

***Note:*** `YOUR_MINIO_ROOT_USER`, `YOUR_MINIO_ROOT_PASSWORD`, `YOUR_NEO4J_USERNAME`, and `YOUR_MINIO_ROOT_PASSWORD` corresponds to your MinIO username, MinIO password, Neo4j username, and Neo4j password, respectively.

Please see [VDMS Test Suite Overview](VDMS-Test-Suite.md) for information on remaining `run_all_tests.py` arguments.
