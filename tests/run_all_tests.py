#!/usr/bin/python3
#
# The MIT License
#
# @copyright Copyright (c) 2024 Intel Corporation
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
import argparse
import json
import shutil
import re
import os
import subprocess
import copy
import signal

from shlex import quote

from abc import ABC, abstractmethod

# Get the path to the tests directory
DEFAULT_CURRENT_DIR = os.getcwd()

# Get the root of the repository
DEFAULT_DIR_REPO = os.path.dirname(DEFAULT_CURRENT_DIR)

DEFAULT_VDMS_APP_PATH = DEFAULT_DIR_REPO + "/build/vdms"

DEFAULT_MINIO_PATH = DEFAULT_DIR_REPO + "/minio"
DEFAULT_MINIO_ALIAS_NAME = "myminio"
DEFAULT_MINIO_PORT = 9000
DEFAULT_MINIO_TMP_DIR = "minio_files"
DEFAULT_MINIO_CONSOLE_PORT = 9001

STOP_ON_FAILURE_FLAG = "--gtest_fail_fast"

DEFAULT_NEO_TEST_PORT = 7687

DEFAULT_NEO_TEST_ENDPOINT = f"neo4j://neo4j:{str(DEFAULT_NEO_TEST_PORT)}"

DEFAULT_GOOGLETEST_PATH = DEFAULT_DIR_REPO + "/build/tests/unit_tests"

DEFAULT_TESTS_STDERR_FILENAME = "tests_stderr_log.log"
DEFAULT_TESTS_STDOUT_FILENAME = "tests_stdout_log.log"

DEFAULT_UDF_LOCAL_STDERR_FILENAME = "udf_local_stderr_log.log"
DEFAULT_UDF_LOCAL_STDOUT_FILENAME = "udf_local_stdout_log.log"

DEFAULT_UDF_SERVER_STDERR_FILENAME = "udf_server_stderr_log.log"
DEFAULT_UDF_SERVER_STDOUT_FILENAME = "udf_server_stdout_log.log"

DEFAULT_TLS_STDERR_FILENAME = "tls_stderr_log.log"
DEFAULT_TLS_STDOUT_FILENAME = "tls_stdout_log.log"

DEFAULT_MINIO_STDERR_FILENAME = "minio_stderr_log.log"
DEFAULT_MINIO_STDOUT_FILENAME = "minio_stdout_log.log"

DEFAULT_VDMS_STDERR_FILENAME = "vdms_stderr_log.log"
DEFAULT_VDMS_STDOUT_FILENAME = "vdms_stdout_log.log"

TYPE_OF_TESTS_AVAILABLE = ["ut", "ru", "pt", "rp", "neo"]
GOOGLETEST_TYPE_OF_TESTS = ["ut", "ru", "neo"]
DEFAULT_TMP_DIR = os.path.join(DEFAULT_CURRENT_DIR, "tests_output_dir")

TESTS_DIRNAME = "tests"

DEFAULT_NEO4J_OPSIO_TEST_FILTER = "OpsIOCoordinatorTest.*"
DEFAULT_NEO4J_OPSIO_CONFIG_FILES = ["unit_tests/config-aws-tests.json"]

DEFAULT_NEO4J_E2E_TEST_FILTER = "OpsIOCoordinatorTest.*"
DEFAULT_NEO4J_E2E_CONFIG_FILES = ["unit_tests/config-neo4j-e2e.json"]

NEO4J_OPS_IO_TEST_TYPE = "OPS_IO"
NEO4J_E2E_TEST_TYPE = "E2E"
NEO4J_BACKEND_TEST_TYPE = "BACKEND"

NEO4J_OPS_IO_REGEX = r"^OpsIOCoordinatorTest\.|^'OpsIOCoordinatorTest\.[^']+'"
NEO4J_E2E_REGEX = r"^Neo4JE2ETest\.|^'Neo4JE2ETest\.[^']+'"
NEO4J_BACKEND_REGEX = r"^Neo4jBackendTest\.|^'Neo4jBackendTest\.[^']+'"

DEFAULT_NON_REMOTE_PYTHON_CONFIG_FILES = [
    "python/config-tests.json",
    "python/config-tls-tests.json",
]

DEFAULT_PYTHON_TEST_FILTER = "discover -s ./python/ --pattern=Test*.py"
DEFAULT_NON_REMOTE_PYTHON_TEST_FILTER = DEFAULT_PYTHON_TEST_FILTER

DEFAULT_NON_REMOTE_UNIT_TEST_FILTER = (
    "-RemoteConnectionTest.*:Neo4jBackendTest.*:OpsIOCoordinatorTest.*:Neo4JE2ETest.*"
)
DEFAULT_NON_REMOTE_UNIT_TEST_CONFIG_FILES = [
    "unit_tests/config-tests.json",
    "unit_tests/config-client-tests.json",
]

DEFAULT_REMOTE_UNIT_TEST_FILTER = "RemoteConnectionTest.*"
DEFAULT_REMOTE_UNIT_TEST_CONFIG_FILES = ["unit_tests/config-aws-tests.json"]

DEFAULT_REMOTE_PYTHON_TEST_FILTER = DEFAULT_PYTHON_TEST_FILTER
DEFAULT_REMOTE_PYTHON_CONFIG_FILES = [
    "python/config-aws-tests.json",
    "python/config-tls-aws-tests.json",
]

DEBUG_MODE = True

# Global variable to keep the tracking of the process objects that are running
processList = []

# Global variable to keep the tracking of all the files open
fdList = []

# Global variable to keep the tracking of where the temporary files are
global_tmp_tests_dir = ""

# Global variable to know if the temporary files need to be deleted once
# the testing finishes or crashes
global_keep_tmp_tests_dir = False


def kill_processes_by_object():
    """
    Kills all processes in the global process list.

    This function iterates over a global list of process objects,
    reverses the list, and kills each process. It also handles exceptions
    and prints debug information if DEBUG_MODE is enabled.

    Global Variables:
    - processList (list): A global list containing process objects to be killed.
    - DEBUG_MODE (bool): A global flag indicating whether debug
                         information should be printed.

    Exceptions:
    - Catches all exceptions and prints a warning message with
      the exception details.
    """
    global processList
    try:

        if DEBUG_MODE:
            print(f"Killing {str(len(processList))} processes")
        processList.reverse()
        for processObject in processList:
            if DEBUG_MODE:
                print(f"Killing pid: {processObject.pid}")
            processObject.kill()

        # Clear the list once all the processes were killed
        processList = []
    except Exception as e:
        print(f"Warning: kill_processes_by_object(): {e}")


def close_log_files():
    """
    Closes all file descriptors in the global file descriptor list.

    This function iterates over a global list of file descriptors, checks
    if each file descriptor is not None and is open, and then closes it.
    It also handles exceptions and prints debug information
    if DEBUG_MODE is enabled.

    Global Variables:
    - fdList (list): A global list containing file descriptors to be closed.
    - DEBUG_MODE (bool): A global flag indicating whether
                         debug information should be printed.

    Exceptions:
    - Catches all exceptions and prints a warning message with the exception
      details.
    """
    try:
        global fdList
        for fd in fdList:
            if fd is not None and (not fd.closed):
                if DEBUG_MODE:
                    print(f"Closing: {fd.name}")
                fd.close()
        # Clear the list once all the files were closed
        fdList = []
    except Exception as e:
        print("Warning: close_log_files(): " + str(e))


def cleanup():
    """
    Cleans up temporary directories based on global flags.

    This function checks the global flag `global_keep_tmp_tests_dir` to
    determine whether to delete the temporary directory specified by
    `global_tmp_tests_dir`. If the directory exists and the flag is set to
    False, the directory is deleted. It also handles exceptions and prints
    debug information if DEBUG_MODE is enabled.

    Global Variables:
    - global_keep_tmp_tests_dir (bool): A flag indicating whether to keep the
      temporary directory.
    - global_tmp_tests_dir (str): The path to the temporary directory to be
      cleaned up.
    - DEBUG_MODE (bool): A global flag indicating whether debug information
      should be printed.

    Exceptions:
    - Raises an exception with a detailed error message if any error occurs
      during cleanup.
    """
    global global_keep_tmp_tests_dir
    global global_tmp_tests_dir
    try:
        if DEBUG_MODE:
            print("Cleaning up")
        # Remove the temporary dir if user set the flag -k to False
        if not global_keep_tmp_tests_dir and global_tmp_tests_dir is not None:
            if os.path.exists(global_tmp_tests_dir):
                if DEBUG_MODE:
                    print("Deleting the directory:", global_tmp_tests_dir)
                shutil.rmtree(global_tmp_tests_dir, ignore_errors=False)
                global_tmp_tests_dir = None
    except Exception as e:
        raise Exception("cleanup() Error: " + str(e))


def signal_handler(sig, frame):
    """
    Handles specific signals and performs cleanup before exiting.

    This function handles signals such as SIGABRT, SIGINT, and SIGSEGV. When
    one of these signals is caught, it prints a debug message (if DEBUG_MODE
    is enabled), closes log files, kills processes, and exits the program
    gracefully. For other signals, it prints a debug message and exits.

    Parameters:
    - sig (int): The signal number.
    - frame (frame object): The current stack frame (not used in this function).

    Global Variables:
    - DEBUG_MODE (bool): A global flag indicating whether debug information
      should be printed.
    """
    if sig in [signal.SIGABRT, signal.SIGINT, signal.SIGSEGV]:
        if DEBUG_MODE:
            print(f"Caught {str(sig)}. Exiting gracefully.")
        close_log_files()
        kill_processes_by_object()
        exit(1)
    else:
        if DEBUG_MODE:
            print(f"Caught signal {str(sig)}. Exiting.")


# Register the signal handlers
signal.signal(signal.SIGABRT, signal_handler)
signal.signal(signal.SIGINT, signal_handler)
signal.signal(signal.SIGSEGV, signal_handler)
signal.signal(signal.SIGTERM, signal_handler)


class TestingArgs:
    test_name: str
    minio_username: str
    minio_password: str
    type_of_test: str
    tmp_tests_dir: str
    config_files_for_vdms: list
    tmp_config_files_for_vdms: list
    minio_app_path: str
    minio_tmp_dir_name: str
    minio_port: int
    minio_alias_name: str
    stop_tests_on_failure: bool
    vdms_app_path: str
    googletest_path: str
    keep_tmp_tests_dir: bool
    stderr_filename: str
    stdout_filename: str
    udf_local_stderr_filename: str
    udf_local_stdout_filename: str
    udf_server_stderr_filename: str
    udf_server_stdout_filename: str
    tls_stderr_filename: str
    tls_stdout_filename: str
    minio_stderr_filename: str
    minio_stdout_filename: str
    vdms_stderr_filename: str
    vdms_stdout_filename: str
    minio_console_port: int
    neo4j_port: int
    neo4j_password: str
    neo4j_username: str
    neo4j_endpoint: str
    run: bool

    def __init__(self):
        self.test_name: str = None
        self.minio_username: str = None
        self.minio_password: str = None
        self.type_of_test: str = None
        self.tmp_tests_dir: str = None
        self.config_files_for_vdms: list = None
        self.tmp_config_files_for_vdms: list = None
        self.minio_app_path: str = None
        self.minio_tmp_dir_name: str = None
        self.minio_port: int = None
        self.minio_alias_name: str = None
        self.stop_tests_on_failure: bool = None
        self.vdms_app_path: str = None
        self.googletest_path: str = None
        self.keep_tmp_tests_dir: bool = None
        self.stderr_filename: str = None
        self.stdout_filename: str = None
        self.udf_local_stderr_filename: str = None
        self.udf_local_stdout_filename: str = None
        self.udf_server_stderr_filename: str = None
        self.udf_server_stdout_filename: str = None
        self.tls_stderr_filename: str = None
        self.tls_stdout_filename: str = None
        self.minio_stderr_filename: str = None
        self.minio_stdout_filename: str = None
        self.vdms_stderr_filename: str = None
        self.vdms_stdout_filename: str = None
        self.minio_console_port: int = None
        self.neo4j_port: int = None
        self.neo4j_password: str = None
        self.neo4j_username: str = None
        self.neo4j_endpoint: str = None
        self.run: bool = None

    def __str__(self):
        """
        Returns the string representation of the object.

        This method returns the string representation of the object by calling
        the `__repr__` method.

        Returns:
        - str: The string representation of the object.
        """
        return self.__repr__()

    def __repr__(self):
        """
        Returns the detailed string representation of the object.

        This method returns a detailed string representation of the object,
        including most of its attributes. Sensitive information like passwords
        is omitted from the representation.

        Returns:
        - str: The detailed string representation of the object.
        """
        class_name = type(self).__name__
        # Omit sensitive information like passwords from the representation
        return (
            f"{class_name}(test_name={self.test_name!r}, minio_username={self.minio_username!r}, "
            f"type_of_test={self.type_of_test!r}, tmp_tests_dir={self.tmp_tests_dir!r}, "
            f"config_files_for_vdms={self.config_files_for_vdms!r}, "
            f"tmp_config_files_for_vdms={self.tmp_config_files_for_vdms!r}, "
            f"minio_app_path={self.minio_app_path!r}, minio_tmp_dir_name={self.minio_tmp_dir_name!r}, "
            f"minio_port={self.minio_port!r}, minio_alias_name={self.minio_alias_name!r}, "
            f"stop_tests_on_failure={self.stop_tests_on_failure!r}, vdms_app_path={self.vdms_app_path!r}, "
            f"googletest_path={self.googletest_path!r}, keep_tmp_tests_dir={self.keep_tmp_tests_dir!r}, "
            f"stderr_filename={self.stderr_filename!r}, stdout_filename={self.stdout_filename!r}, "
            f"udf_local_stderr_filename={self.udf_local_stderr_filename!r}, udf_local_stdout_filename={self.udf_local_stdout_filename!r}, "
            f"udf_server_stderr_filename={self.udf_server_stderr_filename!r}, udf_server_stdout_filename={self.udf_server_stdout_filename!r}, "
            f"tls_stderr_filename={self.tls_stderr_filename!r}, tls_stdout_filename={self.tls_stdout_filename!r}, "
            f"minio_stderr_filename={self.minio_stderr_filename!r}, minio_stdout_filename={self.minio_stdout_filename!r}, "
            f"vdms_stderr_filename={self.vdms_stderr_filename!r}, vdms_stdout_filename={self.vdms_stdout_filename!r}, "
            f"minio_console_port={self.minio_console_port!r}, neo4j_port={self.neo4j_port!r}, "
            f"neo4j_username={self.neo4j_username!r}, neo4j_endpoint={self.neo4j_endpoint!r}, "
            f"run={self.run!r})"
        )


class AbstractTest(ABC):
    """
    Abstract base class for test cases.

    This class defines the interface for test cases, including methods for
    running tests, validating arguments, and filling default arguments. All
    methods must be implemented by subclasses.

    Abstract methods:
    - run(testingArgs: TestingArgs): Runs the test case.
    - validate_arguments(testingArgs: TestingArgs, parser: argparse.ArgumentParser):
      Validates the provided arguments.
    - fill_default_arguments(testingArgs: TestingArgs) -> TestingArgs:
      Fills in default arguments and returns a TestingArgs object.
    """

    @abstractmethod
    def run(self, testingArgs: TestingArgs):
        """
        Runs the test case.

        This method must be implemented by subclasses to define the test
        logic.

        Parameters:
        - testingArgs (TestingArgs): The arguments required to run the test.
        """
        return

    @abstractmethod
    def validate_arguments(
        self, testingArgs: TestingArgs, parser: argparse.ArgumentParser
    ):
        """
        Validates the provided arguments.

        This method must be implemented by subclasses to validate the
        arguments required for the test.

        Parameters:
        - testingArgs (TestingArgs): The arguments to be validated.
        - parser (argparse.ArgumentParser): The argument parser.
        """
        return

    @abstractmethod
    def fill_default_arguments(self, testingArgs: TestingArgs) -> TestingArgs:
        """
        Fills in default arguments and returns a TestingArgs object.

        This method must be implemented by subclasses to provide default
        values for arguments.

        Parameters:
        - testingArgs (TestingArgs): The arguments to be filled with defaults.

        Returns:
        - TestingArgs: The arguments with default values filled in.
        """
        return TestingArgs()

    def get_valid_test_name(
        self, testingArgs: TestingArgs, defaultTestFilter: str
    ) -> str:
        """
        Returns a valid test name based on user input or a default filter.

        This function checks if the user has specified a test name using the
        `-n` flag. If not, it assigns a default test filter to the test name.
        It also prints a warning message if DEBUG_MODE is enabled.

        Parameters:
        - testingArgs (TestingArgs): The arguments provided by the user.
        - defaultTestFilter (str): The default test filter to use if no test name
        is specified.

        Returns:
        - str: The valid test name to be used.
        """
        # If user doesn't specify the -n flag
        if (
            testingArgs.test_name is None
            or testingArgs.test_name == ""
            or testingArgs.test_name == "''"
        ):
            # if the user doesn't specify the tests to be run, then it uses the
            # default filter
            testingArgs.test_name = defaultTestFilter
            if DEBUG_MODE:
                print(
                    "Warning: No test name filter was specified, running the default tests only:",
                    defaultTestFilter,
                )

        return testingArgs.test_name

    def get_valid_vdms_values(
        self, testingArgs: TestingArgs, defaultConfigFiles: list
    ) -> tuple:
        """
        Returns valid VDMS values based on user input or default values.

        This function checks if the user has specified values for `vdms_app_path`
        and `config_files_for_vdms`. If not, it assigns default values to these
        attributes. It returns the validated or default values for VDMS.

        Parameters:
        - testingArgs (TestingArgs): The arguments provided by the user.
        - defaultConfigFiles (list): The default config files to use if no config
        files are specified.

        Returns:
        - tuple: A tuple containing the valid `vdms_app_path` and
        `config_files_for_vdms`.
        """
        # Default value for VDMS
        if (
            not hasattr(testingArgs, "vdms_app_path")
            or testingArgs.vdms_app_path is None
        ):
            testingArgs.vdms_app_path = DEFAULT_VDMS_APP_PATH

        # if the config file is required however, -c flag was not provided
        # set the default config file according to the type of the test
        if (
            not hasattr(testingArgs, "config_files_for_vdms")
            or testingArgs.config_files_for_vdms is None
            or len(testingArgs.config_files_for_vdms) == 0
        ):
            testingArgs.config_files_for_vdms = defaultConfigFiles

        return testingArgs.vdms_app_path, testingArgs.config_files_for_vdms

    def get_valid_minio_values(self, testingArgs: TestingArgs) -> TestingArgs:
        """
        Returns valid MinIO values based on user input or default values.

        This function checks if the user has specified values for `minio_app_path`,
        `minio_tmp_dir_name`, `minio_alias_name`, `minio_port`, and
        `minio_console_port`. If not, it assigns default values to these
        attributes. It returns the validated or default values for MinIO.

        Parameters:
        - testingArgs (TestingArgs): The arguments provided by the user.

        Returns:
        - TestingArgs: The arguments with valid MinIO values.
        """
        # Use the flag "-m MINIO_APP_PATH"
        # This is used for Remote tests only...
        # For specifying the path where the minio app is installed.
        # In case that this flag is not specified
        # then it will use the location where minio app was installed ($PATH)
        if (
            not hasattr(testingArgs, "minio_app_path")
            or testingArgs.minio_app_path is None
        ):
            # if the user doesn't specify the flag -m then it uses the
            # default locations for the binaries ($PATH, /usr/bin, etc)
            testingArgs.minio_app_path = DEFAULT_MINIO_PATH
            if DEBUG_MODE:
                print("Warning: Using default MinIO installation")

        # Temporary dir for files created by MinIO
        testingArgs.minio_tmp_dir_name = (
            testingArgs.tmp_tests_dir + "/" + DEFAULT_MINIO_TMP_DIR
        )

        # Default value for the MinIO alias
        if (
            not hasattr(testingArgs, "minio_alias_name")
            or testingArgs.minio_alias_name is None
            or testingArgs.minio_alias_name == ""
        ):
            testingArgs.minio_alias_name = DEFAULT_MINIO_ALIAS_NAME

        # api_port argument
        if not hasattr(testingArgs, "minio_port") or testingArgs.minio_port is None:
            # if the user doesn't specify the flag -a then it uses the
            # default port or the value obtained from the env var 'AWS_API_PORT'
            minioPortByEnvVar = os.environ.get("AWS_API_PORT")
            if minioPortByEnvVar is None:
                testingArgs.minio_port = DEFAULT_MINIO_PORT
                if DEBUG_MODE:
                    print(
                        "Warning: Using default MinIO port: {minio_port}".format(
                            minio_port=testingArgs.minio_port
                        )
                    )
            else:
                testingArgs.minio_port = minioPortByEnvVar
                if DEBUG_MODE:
                    print(
                        "Warning: Using MinIO port: {minio_port} by using env var".format(
                            minio_port=testingArgs.minio_port
                        )
                    )

        # console_port
        if (
            not hasattr(testingArgs, "minio_console_port")
            or testingArgs.minio_console_port is None
        ):
            # if the user doesn't specify the flag --minio_console_port then it uses the
            # default port or the value obtained from the env var 'AWS_CONSOLE_PORT'
            minioConsolePortByEnvVar = os.environ.get("AWS_CONSOLE_PORT")
            if minioConsolePortByEnvVar is None:
                testingArgs.minio_console_port = DEFAULT_MINIO_CONSOLE_PORT
                if DEBUG_MODE:
                    print(
                        "Warning: Using default MinIO console port: {minio_console_port}".format(
                            minio_console_port=testingArgs.minio_console_port
                        )
                    )
            else:
                testingArgs.minio_console_port = minioConsolePortByEnvVar
                if DEBUG_MODE:
                    print(
                        "Warning: Using MinIO console port: {minio_console_port} by using env var".format(
                            minio_console_port=testingArgs.minio_console_port
                        )
                    )

        return testingArgs

    def get_valid_google_test_values(self, testingArgs: TestingArgs) -> TestingArgs:
        """
        Returns valid Google Test values based on user input or default values.

        This function checks if the user has specified a value for
        `googletest_path`. If not, it assigns a default value to this attribute.
        It returns the validated or default values for Google Test.

        Parameters:
        - testingArgs (TestingArgs): The arguments provided by the user.

        Returns:
        - TestingArgs: The arguments with valid Google Test values.
        """
        # Default value for the app in charge of running the googletest tests
        if testingArgs.googletest_path is None:
            testingArgs.googletest_path = DEFAULT_GOOGLETEST_PATH

        return testingArgs

    def get_valid_neo4j_values(self, testingArgs: TestingArgs) -> TestingArgs:
        """
        Returns valid Neo4j values based on user input or default values.

        This function checks if the user has specified values for `neo4j_port` and
        `neo4j_endpoint`. If not, it assigns default values to these attributes.
        It returns the validated or default values for Neo4j.

        Parameters:
        - testingArgs (TestingArgs): The arguments provided by the user.

        Returns:
        - TestingArgs: The arguments with valid Neo4j values.
        """
        # neo4j_port
        if not hasattr(testingArgs, "neo4j_port") or testingArgs.neo4j_port is None:
            # if the user doesn't specify the flag --neo4j_port then it uses the
            # default neo4j port or the value obtained from the env var 'NEO_TEST_PORT'
            neo4jPortByEnvVar = os.environ.get("NEO_TEST_PORT")
            if neo4jPortByEnvVar is None:
                testingArgs.neo4j_port = DEFAULT_NEO_TEST_PORT
                if DEBUG_MODE:
                    print(
                        "Warning: Using default Neo4j port: {neo4j_port}".format(
                            neo4j_port=testingArgs.neo4j_port
                        )
                    )
            else:
                testingArgs.neo4j_port = neo4jPortByEnvVar
                if DEBUG_MODE:
                    print(
                        "Warning: Using Neo4j port: {neo4j_port} by using env var".format(
                            neo4j_port=testingArgs.neo4j_port
                        )
                    )
        # neo4j_endpoint
        if (
            not hasattr(testingArgs, "neo4j_endpoint")
            or testingArgs.neo4j_endpoint is None
        ):
            # if the user doesn't specify the flag --neo4j_endpoint then it uses the
            # default neo4j endpoint or the value obtained from the env var 'NEO4J_ENDPOINT'
            neo4jEndpointByEnvVar = os.environ.get("NEO4J_ENDPOINT")
            if neo4jEndpointByEnvVar is None:
                testingArgs.neo4j_endpoint = DEFAULT_NEO_TEST_ENDPOINT
                if DEBUG_MODE:
                    print(
                        "Warning: Using default Neo4j endpoint: {neo4j_endpoint}".format(
                            neo4j_endpoint=testingArgs.neo4j_endpoint
                        )
                    )
            else:
                testingArgs.neo4j_endpoint = neo4jEndpointByEnvVar
                if DEBUG_MODE:
                    print(
                        "Warning: Using Neo4j endpoint: {neo4j_endpoint} by using env var".format(
                            neo4j_endpoint=testingArgs.neo4j_endpoint
                        )
                    )

        return testingArgs

    def validate_google_test_path(
        self, testingArgs: TestingArgs, parser: argparse.ArgumentParser
    ):
        """
        Validates the Google Test path provided by the user.

        This function checks if the `googletest_path` attribute in `testingArgs`
        is set and points to an existing path. If the path does not exist or is
        inaccessible, it raises a parser error.

        Parameters:
        - testingArgs (TestingArgs): The arguments provided by the user.
        - parser (argparse.ArgumentParser): The argument parser.
        """
        # Validate -g (googletest_path) argument is set
        if (
            hasattr(testingArgs, "googletest_path")
            and testingArgs.googletest_path is not None
            and testingArgs.googletest_path != ""
        ):
            if not os.path.exists(testingArgs.googletest_path):
                parser.error(
                    "-g/--googletest_path '"
                    + testingArgs.googletest_path
                    + "' does not exist or there is not access to it"
                )

    def validate_minio_values(
        self, testingArgs: TestingArgs, parser: argparse.ArgumentParser
    ):
        """
        Validates the MinIO values provided by the user.

        This function checks if the `minio_username` and `minio_password`
        attributes in `testingArgs` are set. If not, it raises a parser error.
        It also checks if the `minio_app_path` attribute points to an existing
        path. If the path does not exist or is inaccessible, it raises a parser
        error.

        Parameters:
        - testingArgs (TestingArgs): The arguments provided by the user.
        - parser (argparse.ArgumentParser): The argument parser.
        """
        if (
            not hasattr(testingArgs, "minio_username")
            or (testingArgs.minio_username is None or testingArgs.minio_username == "")
        ) or (
            not hasattr(testingArgs, "minio_password")
            or (testingArgs.minio_password is None)
        ):
            parser.error(
                "-t/--type_of_test {type_of_test} was specified.\nHowever, it is missing to specify the minio_username (-u USERNAME) or minio_password (-p PASSWORD) for connecting to the MinIO server".format(
                    type_of_test=testingArgs.type_of_test
                )
            )

        if (
            hasattr(testingArgs, "minio_app_path")
            and testingArgs.minio_app_path is not None
        ):
            if not os.path.exists(testingArgs.minio_app_path):
                parser.error(
                    testingArgs.minio_app_path
                    + " does not exist or there is not access to it"
                )

    def validate_vdms_values(
        self, testingArgs: TestingArgs, parser: argparse.ArgumentParser
    ):
        """
        Validates the VDMS values provided by the user.

        This function checks if the `vdms_app_path` attribute in `testingArgs` is
        set and points to an existing path. If the path does not exist or is
        inaccessible, it raises a parser error. It also validates the
        `config_files_for_vdms` attribute to ensure all specified config files
        exist.

        Parameters:
        - testingArgs (TestingArgs): The arguments provided by the user.
        - parser (argparse.ArgumentParser): The argument parser.
        """
        if (
            hasattr(testingArgs, "vdms_app_path")
            and testingArgs.vdms_app_path is not None
            and testingArgs.vdms_app_path != ""
        ):
            if not os.path.exists(testingArgs.vdms_app_path):
                parser.error(
                    testingArgs.vdms_app_path
                    + " does not exist or there is not access to it"
                )

        # Validation for "config_files_for_vdms" argument, it checks all the config files path exist
        # Note: if config_files_for_vdms is None or empty then it will use the default config files during the call
        # to fill_default_arguments()
        if hasattr(testingArgs, "config_files_for_vdms") and (
            testingArgs.config_files_for_vdms is not None
            and isinstance(testingArgs.config_files_for_vdms, list)
            and len(testingArgs.config_files_for_vdms) > 0
        ):
            for index in range(len(testingArgs.config_files_for_vdms)):
                config_file = testingArgs.config_files_for_vdms[index]
                if not os.path.exists(config_file):
                    parser.error(
                        "Config file: "
                        + config_file
                        + " does not exist or there is not access to it"
                    )

    def run_minio_server(self, testingArgs: TestingArgs, stderrFD, stdoutFD):
        """
        Starts the MinIO server and sets up the necessary configurations.

        This function starts the MinIO server using the specified arguments,
        captures the output, and sets up the necessary MinIO client configurations
        and buckets. It also handles exceptions and prints debug information if
        DEBUG_MODE is enabled.

        Parameters:
        - testingArgs (TestingArgs): The arguments provided by the user.
        - stderrFD: The file descriptor for capturing stderr output.
        - stdoutFD: The file descriptor for capturing stdout output.

        Global Variables:
        - processList (list): A global list to keep track of running processes.
        - DEBUG_MODE (bool): A global flag indicating whether debug information
        should be printed.
        """
        global processList
        try:
            print("Starting MinIO server")

            # Run the command and capture the output
            cmd = [
                testingArgs.minio_app_path,
                "server",
                testingArgs.minio_tmp_dir_name,
                "--address",
                f":{testingArgs.minio_port}",
            ]

            minioProcess = subprocess.Popen(
                cmd, stdout=stdoutFD, stderr=stderrFD, text=True
            )

            if DEBUG_MODE:
                print("Using MinIO server pid:", minioProcess.pid)

            processList.append(minioProcess)
            # Wait for MinIO server to be initialized"
            os.system("sleep 5")

            if DEBUG_MODE:
                print("Creating buckets for the tests")
            # Create the minio-bucket for MinIO
            # by using the corresponding MinIO client which connects to the MinIO server
            # by using its username and password
            subprocess.check_call(
                [
                    "mc",
                    "alias",
                    "set",
                    f"{testingArgs.minio_alias_name}/",
                    f"http://localhost:{testingArgs.minio_port}",
                    testingArgs.minio_username,
                    testingArgs.minio_password,
                ],
                stderr=stderrFD,
                stdout=stdoutFD,
                text=True,
            )

            subprocess.check_call(
                ["mc", "mb", f"{testingArgs.minio_alias_name}/minio-bucket"],
                stderr=stderrFD,
                stdout=stdoutFD,
                text=True,
            )

            os.system("sleep 3")
        except Exception as e:
            raise Exception("run_minio_server() error: " + str(e))

    def run_vdms_server(self, testingArgs: TestingArgs, stderrFD, stdoutFD):
        """
        Starts the VDMS server and sets up the necessary configurations.

        This function starts the VDMS server using the specified arguments,
        captures the output, and sets up the necessary VDMS configurations. It
        also handles exceptions and prints debug information if DEBUG_MODE is
        enabled.

        Parameters:
        - testingArgs (TestingArgs): The arguments provided by the user.
        - stderrFD: The file descriptor for capturing stderr output.
        - stdoutFD: The file descriptor for capturing stdout output.

        Global Variables:
        - processList (list): A global list to keep track of running processes.
        - DEBUG_MODE (bool): A global flag indicating whether debug information
        should be printed.
        """
        global processList
        try:
            print("Starting VDMS server")
            if (
                not hasattr(testingArgs, "tmp_config_files_for_vdms")
                or testingArgs.tmp_config_files_for_vdms is None
            ):
                raise Exception(
                    "run_vdms_server(): tmp_config_files_for_vdms has an invalid value"
                )

            for configFile in testingArgs.tmp_config_files_for_vdms:
                if DEBUG_MODE:
                    print("Using config:", configFile)

                # Run the command and capture the output
                cmd = [testingArgs.vdms_app_path, "-cfg", configFile]

                vdmsProcess = subprocess.Popen(
                    cmd, stdout=stdoutFD, stderr=stderrFD, text=True
                )

                if DEBUG_MODE:
                    print("Using VDMS pid:", vdmsProcess.pid)

                processList.append(vdmsProcess)
                # Wait for VMDS server to be initialized"
                os.system("sleep 3")
        except Exception as e:
            raise Exception("run_vdms_server() error: " + str(e))

    def run_google_tests(self, testingArgs: TestingArgs, stderrFD, stdoutFD):
        """
        Starts and runs Google tests using the specified arguments.

        This function starts the Google tests using the specified arguments,
        captures the output, and handles any errors. It also prints debug
        information if DEBUG_MODE is enabled.

        Parameters:
        - testingArgs (TestingArgs): The arguments provided by the user.
        - stderrFD: The file descriptor for capturing stderr output.
        - stdoutFD: The file descriptor for capturing stdout output.

        Global Variables:
        - processList (list): A global list to keep track of running processes.
        - DEBUG_MODE (bool): A global flag indicating whether debug information
        should be printed.
        """
        global processList
        try:
            stop_on_failure_value = ""
            if testingArgs.stop_tests_on_failure:
                stop_on_failure_value = STOP_ON_FAILURE_FLAG

            print(
                "Starting Google tests: {test_filter}...".format(
                    test_filter=testingArgs.test_name
                )
            )

            # Run the command and capture the output
            cmd = [
                testingArgs.googletest_path,
                f"--gtest_filter={testingArgs.test_name}",
            ]
            if stop_on_failure_value != "":
                cmd.append(stop_on_failure_value)

            subprocess.run(
                cmd, text=True, check=True
            )

        except Exception as e:
            raise Exception("run_google_tests() error: " + str(e))

    def open_log_files(self, tmp_tests_dir: str, stderr: str, stdout: str) -> tuple:
        """
        Opens log files for capturing stderr and stdout output.

        This function opens log files for capturing stderr and stdout output,
        stores the file descriptors in a global list, and returns the file
        descriptors.

        Parameters:
        - tmp_tests_dir (str): The directory where the log files will be created.
        - stderr (str): The name of the stderr log file.
        - stdout (str): The name of the stdout log file.

        Returns:
        - tuple: A tuple containing the file descriptors for stderr and stdout.

        Global Variables:
        - fdList (list): A global list to keep track of open file descriptors.
        """
        global fdList
        stderrFD = open(os.path.join(tmp_tests_dir, stderr), "w")
        fdList.append(stderrFD)

        stdoutFD = open(os.path.join(tmp_tests_dir, stdout), "w")
        fdList.append(stdoutFD)

        return stderrFD, stdoutFD

    def set_values_for_python_client(self):
        """
        Sets the PYTHONPATH environment variable for the Python client.

        This function sets the PYTHONPATH environment variable to include the
        path to the Python client and the tests directory. It also handles
        exceptions and prints debug information if DEBUG_MODE is enabled.

        Global Variables:
        - DEBUG_MODE (bool): A global flag indicating whether debug information
        should be printed.
        """
        # Get path to the client dir
        clientPath = f"{DEFAULT_DIR_REPO}/client/python"
        if not os.path.exists(clientPath):
            raise Exception(
                "Path to the Python client: {path} is invalid or you don't have the permissions to access it".format(
                    path=clientPath
                )
            )
        pythonPath = os.environ.get("PYTHONPATH")
        # Appends the path to the client
        if pythonPath is not None:
            os.environ["PYTHONPATH"] = pythonPath + ":" + clientPath
        else:
            os.environ["PYTHONPATH"] = clientPath

        os.environ["PYTHONPATH"] = (
            os.environ.get("PYTHONPATH") + ":" + DEFAULT_DIR_REPO + "/tests/python"
        )

        if DEBUG_MODE:
            print("PYTHONPATH:", os.environ.get("PYTHONPATH"))

    def run_prep_certs_script(self, stderrFD, stdoutFD):
        """
        Runs the prep_certs.py script to generate certificates for TLS tests.

        This function runs the `prep_certs.py` script located in the TLS tests
        directory to generate the necessary certificates for TLS tests. It also
        handles exceptions and prints debug information if DEBUG_MODE is enabled.

        Parameters:
        - stderrFD: The file descriptor for capturing stderr output.
        - stdoutFD: The file descriptor for capturing stdout output.
        """
        try:
            # Run the prep for the TLS tests to generate certificates
            print("run_prep_certs_script...")

            prepCerts = f"{DEFAULT_DIR_REPO}/tests/tls_test/prep_certs.py"
            if not os.path.exists(prepCerts):
                raise Exception(f"{prepCerts} is an invalid file")

            subprocess.check_call(
                f"python3 {prepCerts}",
                shell=True,
                stderr=stderrFD,
                stdout=stdoutFD,
                text=True,
            )

        except Exception as e:
            raise Exception("run_prep_certs_script() error: " + str(e))

    def run_python_tests(self, testingArgs: TestingArgs, stderrFD, stdoutFD):
        """
        Runs Python tests using the specified arguments.

        This function runs Python tests using the specified arguments, captures
        the output, and handles any errors. It also prints debug information if
        DEBUG_MODE is enabled.

        Parameters:
        - testingArgs (TestingArgs): The arguments provided by the user.
        - stderrFD: The file descriptor for capturing stderr output.
        - stdoutFD: The file descriptor for capturing stdout output.

        Global Variables:
        - processList (list): A global list to keep track of running processes.
        - DEBUG_MODE (bool): A global flag indicating whether debug information
        should be printed.
        """
        global processList

        pythonProcess = None
        try:
            print("Running Python tests...")
            print("Test filter:", testingArgs.test_name)

            # To avoid BASH injection, the test_name is escaped
            test_name = testingArgs.test_name
            if testingArgs.test_name != DEFAULT_PYTHON_TEST_FILTER:
                test_name = quote(testingArgs.test_name)

            # Run the command and capture the output
            cmd = f'python3 -m coverage run -a --include="{DEFAULT_DIR_REPO}/*" --omit="{DEFAULT_DIR_REPO}/client/python/vdms/queryMessage_pb2.py,{DEFAULT_DIR_REPO}/tests/*" -m unittest'
            cmd = cmd + " " + test_name
            cmd = cmd + " -v"
            pythonProcess = subprocess.Popen(
                cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True
            )
            stdout, stderr = pythonProcess.communicate()

            # Decode the byte strings
            output = stdout.decode("utf-8")
            errors = stderr.decode("utf-8")

            # Print the command output
            self.write_to_fd("stdout", "run_python_tests", output, stdoutFD, DEBUG_MODE)

            # Check for errors
            if errors:
                self.write_to_fd(
                    "stderr", "run_python_tests", errors, stderrFD, DEBUG_MODE
                )

            if DEBUG_MODE:
                print("Using python3 pid for tests:", pythonProcess.pid)

            processList.append(pythonProcess)

        except Exception as e:
            if pythonProcess is not None:
                pythonProcess.kill()
            raise Exception("run_python_tests() error: " + str(e))

    def write_to_fd(self, message_type, function_name, message, writer, verbose=False):
        """
        Writes a message to a file descriptor and optionally prints it.

        This function writes a message to a specified file descriptor and
        optionally prints the message with additional context if verbose mode is
        enabled. It also handles exceptions.

        Parameters:
        - message_type (str): The type of message (e.g., "stdout", "stderr").
        - function_name (str): The name of the function generating the message.
        - message (str): The message to be written.
        - writer: The file descriptor to write the message to.
        - verbose (bool): If True, prints the message with additional context.
        """
        try:
            if verbose:
                print(
                    f"**************Beginning of {message_type} logs in {function_name}()*************"
                )

            print(message)
            writer.write(message)

            if verbose:
                print(
                    f"**************End of {message_type} logs in {function_name}()*******************"
                )
        except Exception as e:
            raise Exception("write_to_fd() error: " + str(e))


# Equivalent to run_neo4j_tests.sh
class Neo4jTest(AbstractTest):
    """
    A class to handle Neo4j tests.

    This class provides methods to validate Neo4j values and run Neo4j tests.
    It extends the AbstractTest class.
    """

    def validate_neo4j_values(
        self, testingArgs: TestingArgs, parser: argparse.ArgumentParser
    ):
        """
        Validates the Neo4j values provided by the user.

        This method checks if the `neo4j_username` and `neo4j_password`
        attributes in `testingArgs` are set. If not, it raises a parser error.

        Parameters:
        - testingArgs (TestingArgs): The arguments provided by the user.
        - parser (argparse.ArgumentParser): The argument parser.
        """

        if (
            not hasattr(testingArgs, "neo4j_username")
            or (testingArgs.neo4j_username is None or testingArgs.neo4j_username == "")
        ) or (
            not hasattr(testingArgs, "neo4j_password")
            or (testingArgs.neo4j_password is None)
        ):
            parser.error(
                "-t/--type_of_test {type_of_test} was specified.\nHowever, it is missing to specify the neo4j_username (--neo4j_username) or neo4j_password (--neo4j_password) for connecting to the Neo4j server".format(
                    type_of_test=testingArgs.type_of_test
                )
            )

    def get_type_of_neo_test(self, test_name: str) -> str:
        """
        Determines the type of Neo4j test based on the test name.

        This method uses regular expressions to match the test name against
        predefined patterns to determine the type of Neo4j test.

        Parameters:
        - test_name (str): The name of the test.

        Returns:
            str: The type of Neo4j test. Possible values are:
                - `NEO4J_OPS_IO_TEST_TYPE` for OpsIOCoordinatorTest
                - `NEO4J_E2E_TEST_TYPE` for Neo4JE2ETest
                - `NEO4J_BACKEND_TEST_TYPE` for Neo4jBackendTest
                - `None` if the test name does not match any known pattern.
        """

        if re.match(NEO4J_OPS_IO_REGEX, test_name):
            type = NEO4J_OPS_IO_TEST_TYPE
        elif re.match(NEO4J_E2E_REGEX, test_name):
            type = NEO4J_E2E_TEST_TYPE
        elif re.match(NEO4J_BACKEND_REGEX, test_name):
            type = NEO4J_BACKEND_TEST_TYPE
        else:
            type = None

        return type

    def fill_default_arguments(self, testingArgs: TestingArgs) -> TestingArgs:
        """
        Fills in default arguments for the Neo4jTest object

        This function fills in default arguments for the Neo4jTest object
        based on the specified test name. It sets default values for test name
        filters, VDMS configurations, MinIO values, and Neo4j configurations. It
        also handles exceptions.

        Parameters:
        - testingArgs (TestingArgs): The arguments provided by the user.

        Returns:
        - TestingArgs: The arguments with default values filled in.
        """

        try:
            # Using the flag "-n YOUR_TEST_NAME" for specifying the test name filter.
            # In case that this flag is not specified then it will use the default
            # filter pattern
            testingArgs.test_name = self.get_valid_test_name(
                testingArgs, DEFAULT_NEO4J_OPSIO_TEST_FILTER
            )

            # Get the type of Neo test
            neo_test_type = self.get_type_of_neo_test(testingArgs.test_name)

            if (
                neo_test_type == NEO4J_OPS_IO_TEST_TYPE
                or neo_test_type == NEO4J_E2E_TEST_TYPE
            ):
                # Use the flag "-c CONFIG_FILEPATH_1 -c CONFIG_FILEPATH_2"
                # Note: It will create an instance of VDMS server per each occurrence of
                # -c with its corresponding path to the config file.
                # In case this flag is not specified then it will use the following config
                # files as default
                # For "neo" tests: 'unit_tests/config-neo4j-e2e.json'
                if neo_test_type == NEO4J_E2E_TEST_TYPE:
                    testingArgs.vdms_app_path, testingArgs.config_files_for_vdms = (
                        self.get_valid_vdms_values(
                            testingArgs, DEFAULT_NEO4J_E2E_CONFIG_FILES
                        )
                    )
                else:
                    testingArgs.vdms_app_path, testingArgs.config_files_for_vdms = (
                        self.get_valid_vdms_values(
                            testingArgs, DEFAULT_NEO4J_OPSIO_CONFIG_FILES
                        )
                    )

                testingArgs = self.get_valid_minio_values(testingArgs)

                # Set minio bucket name
                if neo_test_type == NEO4J_OPS_IO_TEST_TYPE:
                    testingArgs.minio_alias_name = "opsio_tester"
                elif neo_test_type == NEO4J_E2E_TEST_TYPE:
                    testingArgs.minio_alias_name = "e2e_tester"
            elif neo_test_type == NEO4J_BACKEND_TEST_TYPE:
                testingArgs.config_files_for_vdms = []
            else:
                testingArgs.config_files_for_vdms = []

            # Fill the default values for running the googletest tests
            testingArgs = self.get_valid_google_test_values(testingArgs)

            if (
                neo_test_type == NEO4J_BACKEND_TEST_TYPE
                or neo_test_type == NEO4J_E2E_TEST_TYPE
            ):

                # Fill the default values for Neo4j configuration
                testingArgs = self.get_valid_neo4j_values(testingArgs)

                os.environ["NEO_TEST_PORT"] = str(testingArgs.neo4j_port)
                os.environ["NEO4J_USER"] = testingArgs.neo4j_username
                os.environ["NEO4J_PASS"] = testingArgs.neo4j_password
                os.environ["NEO4J_ENDPOINT"] = testingArgs.neo4j_endpoint

        except Exception as e:
            raise Exception("fill_default_arguments() in Neo4jTest() error: " + str(e))

        return testingArgs

    def validate_arguments(
        self, testingArgs: TestingArgs, parser: argparse.ArgumentParser
    ):
        """
        Validates the arguments provided by the user for Neo4j tests.

        This function performs common validation for all Neo4j tests. It checks
        if the parser is provided, validates the path to the binary for running
        Google tests, and validates specific arguments based on the test name.

        Parameters:
        - testingArgs (TestingArgs): The arguments provided by the user.
        - parser (argparse.ArgumentParser): The argument parser.

        Raises:
        - Exception: If the parser is None, or if the test name is invalid or not
        provided, or if specific required arguments are missing.
        """
        try:
            # Common validation for all the Neo4j tests
            if parser is None:
                raise Exception("parser is None")

            # Validate the path to the binary in charge of running the googletest tests
            self.validate_google_test_path(testingArgs, parser)

            if hasattr(testingArgs, "test_name") and testingArgs.test_name is not None:
                # Get the type of Neo test
                neo_test_type = self.get_type_of_neo_test(testingArgs.test_name)

                # Validation for OpsIOCoordinatorTest and Neo4JE2ETest only
                if (
                    neo_test_type == NEO4J_OPS_IO_TEST_TYPE
                    or neo_test_type == NEO4J_E2E_TEST_TYPE
                ):
                    # Test requires MinIO & Neo4J Container
                    # Common validations for OpsIOCoordinatorTest and Neo4JE2ETest

                    # Validate MinIO arguments
                    self.validate_minio_values(testingArgs, parser)

                    # Validation for Neo4JE2ETest
                    if neo_test_type == NEO4J_E2E_TEST_TYPE:
                        # Test requires Neo4J Container ONLY
                        self.validate_neo4j_values(testingArgs, parser)

                        # Validate the VDMS arguments
                        self.validate_vdms_values(testingArgs, parser)

                elif neo_test_type == NEO4J_BACKEND_TEST_TYPE:
                    # Test requires Neo4J Container ONLY
                    self.validate_neo4j_values(testingArgs, parser)
                else:
                    raise Exception(
                        "test_name value is invalid:{test_name}".format(
                            test_name=testingArgs.test_name
                        )
                    )
            else:
                raise Exception("test_name value was not provided or it is invalid")
        except Exception as e:
            raise Exception("validate_arguments() in Neo4jTest error: " + str(e))

    def run(self, testingArgs: TestingArgs):
        """
        Executes the Neo4j tests based on the provided arguments.

        This function opens log files, starts necessary servers (MinIO and VDMS),
        and runs the Google tests based on the specified test name. It also
        handles exceptions and prints debug information if DEBUG_MODE is enabled.

        Parameters:
        - testingArgs (TestingArgs): The arguments provided by the user.

        Raises:
        - Exception: If any error occurs during the execution of the tests.
        """

        testsStderrFD = ""
        testsStdoutFD = ""
        minioStderrFD = ""
        minioStdoutFD = ""
        vdmsStderrFD = ""
        vdmsStdoutFD = ""

        try:
            # Open the log files
            testsStderrFD, testsStdoutFD = self.open_log_files(
                testingArgs.tmp_tests_dir,
                testingArgs.stderr_filename,
                testingArgs.stdout_filename,
            )

            minioStderrFD, minioStdoutFD = self.open_log_files(
                testingArgs.tmp_tests_dir,
                testingArgs.minio_stderr_filename,
                testingArgs.minio_stdout_filename,
            )

            vdmsStderrFD, vdmsStdoutFD = self.open_log_files(
                testingArgs.tmp_tests_dir,
                testingArgs.vdms_stderr_filename,
                testingArgs.vdms_stdout_filename,
            )

            # Get the type of Neo test
            neo_test_type = self.get_type_of_neo_test(testingArgs.test_name)

            if (
                neo_test_type == NEO4J_OPS_IO_TEST_TYPE
                or neo_test_type == NEO4J_E2E_TEST_TYPE
            ):
                # Start the MinIO Server and create the bucket
                self.run_minio_server(testingArgs, minioStderrFD, minioStdoutFD)

            if neo_test_type == NEO4J_E2E_TEST_TYPE:
                # Start an instance of the VDMS server per each config file given as argument
                self.run_vdms_server(testingArgs, vdmsStderrFD, vdmsStdoutFD)

            # If the argument "run" is True then it executes the tests
            # (By default the value is True)
            if hasattr(testingArgs, "run") and testingArgs.run:
                # Run the Googletest tests
                self.run_google_tests(testingArgs, testsStderrFD, testsStdoutFD)

            print("Tests Complete!")

        except Exception as e:
            raise Exception("run() Exception in Neo4jTest:" + str(e))


# Equivalent to run_tests.sh
class NonRemoteTest(AbstractTest):
    """
    A class to handle non-remote C++ tests.

    This class provides methods to set up requirements for the remote UDF
    server and run non-remote tests. It extends the AbstractTest class.
    """

    def setup_requirements_for_remote_udf_server(self, stderrFD, stdoutFD):
        """
        Sets up the requirements for the remote UDF server.

        This method installs the necessary Python packages for the remote UDF
        server by running the `pip install` command with the requirements file.
        It also handles exceptions and prints debug information if DEBUG_MODE
        is enabled.

        Parameters:
        - stderrFD: The file descriptor for capturing stderr output.
        - stdoutFD: The file descriptor for capturing stdout output.

        Raises:
        - Exception: If any error occurs during the setup process.
        """

        try:

            subprocess.run(
                f"python -m pip install -r {DEFAULT_DIR_REPO}/remote_function/requirements.txt",
                shell=True,
                stderr=stderrFD,
                stdout=stdoutFD,
                text=True,
                check=True
            )

        except Exception as e:
            raise Exception(
                "setup_requirements_for_remote_udf_server() error: " + str(e)
            )

    def run_remote_udf_server(self, tmp_dir, stderrFD, stdoutFD):
        """
        Runs the remote UDF server.

        This method starts the remote UDF server using the specified arguments,
        captures the output, and handles any errors. It also prints debug
        information if DEBUG_MODE is enabled.

        Parameters:
        - tmp_dir: The temporary directory for the UDF server.
        - stderrFD: The file descriptor for capturing stderr output.
        - stdoutFD: The file descriptor for capturing stdout output.

        Global Variables:
        - processList (list): A global list to keep track of running processes.
        - DEBUG_MODE (bool): A global flag indicating whether debug information
        should be printed.

        Raises:
        - Exception: If any error occurs during the execution of the UDF server.
        """
        global processList
        pythonProcess = None
        try:
            udfServer = f"{DEFAULT_DIR_REPO}/tests/remote_function_test/udf_server.py"
            if not os.path.exists(udfServer):
                raise Exception(f"{udfServer} is an invalid file")

            pythonProcess = subprocess.Popen(
                [
                    "python3",
                    udfServer,
                    "5010",
                    f"{DEFAULT_DIR_REPO}/tests/remote_function_test/functions",
                    tmp_dir,
                ],
                stderr=stderrFD,
                stdout=stdoutFD,
                text=True,
            )

            if DEBUG_MODE:
                print("Using python3 pid:", pythonProcess.pid)

            processList.append(pythonProcess)

            # Wait for UDF server to be initialized"
            os.system("sleep 5")

        except Exception as e:
            raise Exception("run_remote_udf_server() error: " + str(e))

    def setup_for_remote_udf_server_tests(self, tmp_dir, stderrFD, stdoutFD):
        """
        Sets up the environment for remote UDF server tests.

        This method sets up the requirements for the remote UDF server and then
        starts the UDF server using the specified arguments. It also handles
        exceptions and prints debug information if DEBUG_MODE is enabled.

        Parameters:
        - tmp_dir: The temporary directory for the UDF server.
        - stderrFD: The file descriptor for capturing stderr output.
        - stdoutFD: The file descriptor for capturing stdout output.

        Raises:
        - Exception: If any error occurs during the setup process.
        """
        try:
            print("setup_for_remote_udf_server_tests...")

            self.setup_requirements_for_remote_udf_server(stderrFD, stdoutFD)
            self.run_remote_udf_server(tmp_dir, stderrFD, stdoutFD)
        except Exception as e:
            raise Exception(
                "setup_for_remote_udf_server_tests() in NonRemoteTest() error: "
                + str(e)
            )

    def setup_requirements_for_local_udf_message_queue(self, stderrFD, stdoutFD):
        """
        Sets up the requirements for the local UDF message queue.

        This method installs the necessary Python packages for the local UDF
        message queue by running the `pip install` command with the requirements
        file. It also handles exceptions and prints debug information if
        DEBUG_MODE is enabled.

        Parameters:
        - stderrFD: The file descriptor for capturing stderr output.
        - stdoutFD: The file descriptor for capturing stdout output.

        Raises:
        - Exception: If any error occurs during the setup process.
        """
        try:

            subprocess.run(
                f"python -m pip install -r {DEFAULT_DIR_REPO}/user_defined_operations/requirements.txt",
                shell=True,
                stderr=stderrFD,
                stdout=stdoutFD,
                text=True,
                check=True
            )

        except Exception as e:
            raise Exception(
                "setup_requirements_for_local_udf_message_queue() error: " + str(e)
            )

    def run_local_udf_message_queue(self, tmp_dir, stderrFD, stdoutFD):
        """
        Runs the local UDF message queue.

        This method starts the local UDF message queue using the specified
        arguments, captures the output, and handles any errors. It also prints
        debug information if DEBUG_MODE is enabled.

        Parameters:
        - tmp_dir: The temporary directory for the UDF message queue.
        - stderrFD: The file descriptor for capturing stderr output.
        - stdoutFD: The file descriptor for capturing stdout output.

        Global Variables:
        - processList (list): A global list to keep track of running processes.
        - DEBUG_MODE (bool): A global flag indicating whether debug information
        should be printed.

        Raises:
        - Exception: If any error occurs during the execution of the UDF message
        queue.
        """
        global processList
        pythonProcess = None
        try:
            udfLocal = f"{DEFAULT_DIR_REPO}/tests/udf_test/udf_local.py"
            if not os.path.exists(udfLocal):
                raise Exception(f"{udfLocal} is an invalid file")

            pythonProcess = subprocess.Popen(
                [
                    "python3",
                    udfLocal,
                    f"{DEFAULT_DIR_REPO}/tests/udf_test/functions",
                    f"{DEFAULT_DIR_REPO}/tests/udf_test/settings.json",
                    tmp_dir,
                ],
                stderr=stderrFD,
                stdout=stdoutFD,
                text=True,
            )

            if DEBUG_MODE:
                print("Using python3 pid:", pythonProcess.pid)
            processList.append(pythonProcess)

        except Exception as e:
            raise Exception("run_local_udf_message_queue() error: " + str(e))

    def setup_for_local_udf_message_queue_tests(self, tmp_dir, stderrFD, stdoutFD):
        """
        Sets up the environment for local UDF message queue tests.

        This method sets up the requirements for the local UDF message queue and
        then starts the UDF message queue using the specified arguments. It also
        handles exceptions and prints debug information if DEBUG_MODE is enabled.

        Parameters:
        - tmp_dir: The temporary directory for the UDF message queue.
        - stderrFD: The file descriptor for capturing stderr output.
        - stdoutFD: The file descriptor for capturing stdout output.

        Raises:
        - Exception: If any error occurs during the setup process.
        """

        try:
            print("setup_for_local_udf_message_queue_tests...")

            self.setup_requirements_for_local_udf_message_queue(stderrFD, stdoutFD)
            self.run_local_udf_message_queue(tmp_dir, stderrFD, stdoutFD)

        except Exception as e:
            raise Exception(
                "setup_for_local_udf_message_queue_tests() in NonRemoteTest() error: "
                + str(e)
            )

    def fill_default_arguments(self, testingArgs: TestingArgs) -> TestingArgs:
        """
        Fills in default arguments for the NonRemoteTest object.

        This method fills in default arguments for the NonRemoteTest object
        based on the specified test name. It sets default values for test name
        filters, VDMS configurations, and Google test values.
        It also handles exceptions.

        Parameters:
        - testingArgs (TestingArgs): The arguments provided by the user.

        Returns:
        - TestingArgs: The arguments with default values filled in.

        Raises:
        - Exception: If any error occurs during the process of filling default
        arguments.
        """
        try:
            # Using the flag "-n YOUR_TEST_NAME" for specifying the test name filter.
            # In case that this flag is not specified then it will use the default
            # filter pattern
            testingArgs.test_name = self.get_valid_test_name(
                testingArgs, DEFAULT_NON_REMOTE_UNIT_TEST_FILTER
            )

            # Use the flag "-c CONFIG_FILEPATH_1 -c CONFIG_FILEPATH_2"
            # Note: It will create an instance of VDMS server per each occurrence of
            # -c with its corresponding path to the config file.
            # In case this flag is not specified then it will use the following config
            # files as default
            # For "ut" tests: unit_tests/config-tests.json and unit_tests/config-client-tests.json
            testingArgs.vdms_app_path, testingArgs.config_files_for_vdms = (
                self.get_valid_vdms_values(
                    testingArgs, DEFAULT_NON_REMOTE_UNIT_TEST_CONFIG_FILES
                )
            )

            # Fill the default values for running the googletest tests
            testingArgs = self.get_valid_google_test_values(testingArgs)

        except Exception as e:
            raise Exception(
                "fill_default_arguments() in NonRemoteTest() error: " + str(e)
            )

        return testingArgs

    def validate_arguments(
        self, testingArgs: TestingArgs, parser: argparse.ArgumentParser
    ):
        """
        Validates the arguments provided by the user for non-remote tests.

        This method performs validation for the VDMS arguments and the path to
        the binary for running Google tests. It also handles exceptions.

        Parameters:
        - testingArgs (TestingArgs): The arguments provided by the user.
        - parser (argparse.ArgumentParser): The argument parser.

        Raises:
        - Exception: If the parser is None or if any validation fails.
        """
        try:
            if parser is None:
                raise Exception("parser is None")

            # Validate the VDMS arguments
            self.validate_vdms_values(testingArgs, parser)

            # Validate the path to the binary in charge of running the googletest tests
            self.validate_google_test_path(testingArgs, parser)

        except Exception as e:
            raise Exception("validate_arguments() in NonRemoteTest error: " + str(e))

    def run(self, testingArgs: TestingArgs):
        """
        Executes the non-remote C++ tests based on the provided arguments.

        This method opens log files, sets up the environment for remote UDF
        server tests, starts the UDF message queue, prepares the TLS environment,
        starts the VDMS server, and runs the Google tests based on the specified
        test name. It also handles exceptions and prints debug information if
        DEBUG_MODE is enabled.

        Parameters:
        - testingArgs (TestingArgs): The arguments provided by the user.

        Raises:
        - Exception: If any error occurs during the execution of the tests.
        """

        testsStderrFD = ""
        testsStdoutFD = ""
        udfLocalStderrFD = ""
        udfLocalStdoutFD = ""
        udfServerStderrFD = ""
        udfServerStdoutFD = ""
        tlsStderrFD = ""
        tlsStdoutFD = ""
        vdmsStderrFD = ""
        vdmsStdoutFD = ""

        try:
            # Open the log files the servers
            testsStderrFD, testsStdoutFD = self.open_log_files(
                testingArgs.tmp_tests_dir,
                testingArgs.stderr_filename,
                testingArgs.stdout_filename,
            )

            udfLocalStderrFD, udfLocalStdoutFD = self.open_log_files(
                testingArgs.tmp_tests_dir,
                testingArgs.udf_local_stderr_filename,
                testingArgs.udf_local_stdout_filename,
            )

            udfServerStderrFD, udfServerStdoutFD = self.open_log_files(
                testingArgs.tmp_tests_dir,
                testingArgs.udf_server_stderr_filename,
                testingArgs.udf_server_stdout_filename,
            )

            tlsStderrFD, tlsStdoutFD = self.open_log_files(
                testingArgs.tmp_tests_dir,
                testingArgs.tls_stderr_filename,
                testingArgs.tls_stdout_filename,
            )

            vdmsStderrFD, vdmsStdoutFD = self.open_log_files(
                testingArgs.tmp_tests_dir,
                testingArgs.vdms_stderr_filename,
                testingArgs.vdms_stdout_filename,
            )

            # Start remote UDF server for test
            self.setup_for_remote_udf_server_tests(
                testingArgs.tmp_tests_dir, udfServerStderrFD, udfServerStdoutFD
            )

            # Start UDF message queue for test
            self.setup_for_local_udf_message_queue_tests(
                testingArgs.tmp_tests_dir, udfLocalStderrFD, udfLocalStdoutFD
            )

            # Prepare the TLS environment for testing
            self.run_prep_certs_script(tlsStderrFD, tlsStdoutFD)

            # Start an instance of the VDMS server per each config file given as argument
            self.run_vdms_server(testingArgs, vdmsStderrFD, vdmsStdoutFD)

            # If the argument "run" is True then it executes the tests
            # (By default the value is True)
            if hasattr(testingArgs, "run") and testingArgs.run:
                # Run the Googletest tests
                self.run_google_tests(testingArgs, testsStderrFD, testsStdoutFD)

        except Exception as e:
            raise Exception("run() Exception in NonRemoteTest: " + str(e))

        print("Finished")


# Equivalent to run_python_tests.sh
class NonRemotePythonTest(AbstractTest):
    """
    A class to handle non-remote Python tests.

    This class provides methods to set up the environment and run non-remote
    Python tests. It extends the AbstractTest class.
    """

    def validate_arguments(self, testingArgs: TestingArgs, parser):
        """
        Validates the arguments provided by the user for non-remote Python tests.

        This method performs validation for the VDMS arguments and the path to
        the binary for running Google tests. It also handles exceptions.

        Parameters:
        - testingArgs (TestingArgs): The arguments provided by the user.
        - parser (argparse.ArgumentParser): The argument parser.

        Raises:
        - Exception: If the parser is None or if any validation fails.
        """

        try:
            if parser is None:
                raise Exception("parser is None")

            # Validate the VDMS arguments
            self.validate_vdms_values(testingArgs, parser)

            # Validate the path to the binary in charge of running the googletest tests
            self.validate_google_test_path(testingArgs, parser)

        except Exception as e:
            raise Exception(
                "validate_arguments() in NonRemotePythonTest error: " + str(e)
            )

    def fill_default_arguments(self, testingArgs: TestingArgs) -> TestingArgs:
        """
        Fills in default arguments for the NonRemotePythonTest object.

        This method fills in default arguments for the NonRemotePythonTest object
        based on the specified test name. It sets default values for test name
        filters and VDMS configurations. It also handles exceptions.

        Parameters:
        - testingArgs (TestingArgs): The arguments provided by the user.

        Returns:
        - TestingArgs: The arguments with default values filled in.

        Raises:
        - Exception: If any error occurs during the process of filling default
        arguments.
        """
        try:
            # Using the flag "-n YOUR_TEST_NAME" for specifying the test name filter.
            # In case that this flag is not specified then it will use the default
            # filter pattern
            testingArgs.test_name = self.get_valid_test_name(
                testingArgs, DEFAULT_NON_REMOTE_PYTHON_TEST_FILTER
            )

            # Use the flag "-c CONFIG_FILEPATH_1 -c CONFIG_FILEPATH_2"
            # Note: It will create an instance of VDMS server per each occurrence of
            # -c with its corresponding path to the config file.
            # In case this flag is not specified then it will use the config
            # files defined as default
            testingArgs.vdms_app_path, testingArgs.config_files_for_vdms = (
                self.get_valid_vdms_values(
                    testingArgs, DEFAULT_NON_REMOTE_PYTHON_CONFIG_FILES
                )
            )

        except Exception as e:
            raise Exception(
                "fill_default_arguments() in NonRemotePythonTest() error: " + str(e)
            )

        return testingArgs

    def run(self, testingArgs: TestingArgs):
        """
        Executes the non-remote Python tests based on the provided arguments.

        This method sets the values for the Python client, opens log files,
        prepares the TLS environment, starts the VDMS server, and runs the Python
        tests based on the specified test name. It also handles exceptions and
        prints debug information if DEBUG_MODE is enabled.

        Parameters:
        - testingArgs (TestingArgs): The arguments provided by the user.

        Raises:
        - Exception: If any error occurs during the execution of the tests.
        """
        testsStderrFD = ""
        testsStdoutFD = ""
        tlsStderrFD = ""
        tlsStdoutFD = ""
        vdmsStderrFD = ""
        vdmsStdoutFD = ""

        try:
            # Set the values for the Python client
            self.set_values_for_python_client()

            # Open the log files
            testsStderrFD, testsStdoutFD = self.open_log_files(
                testingArgs.tmp_tests_dir,
                testingArgs.stderr_filename,
                testingArgs.stdout_filename,
            )

            tlsStderrFD, tlsStdoutFD = self.open_log_files(
                testingArgs.tmp_tests_dir,
                testingArgs.tls_stderr_filename,
                testingArgs.tls_stdout_filename,
            )

            vdmsStderrFD, vdmsStdoutFD = self.open_log_files(
                testingArgs.tmp_tests_dir,
                testingArgs.vdms_stderr_filename,
                testingArgs.vdms_stdout_filename,
            )

            # Prepare the TLS environment for testing
            self.run_prep_certs_script(tlsStderrFD, tlsStdoutFD)

            # Start an instance of the VDMS server per each config file given as argument
            self.run_vdms_server(testingArgs, vdmsStderrFD, vdmsStdoutFD)

            # If the argument "run" is True then it executes the tests
            # (By default the value is True)
            if hasattr(testingArgs, "run") and testingArgs.run:
                # Run the Python tests
                self.run_python_tests(testingArgs, testsStderrFD, testsStdoutFD)

            print("Finished")

        except Exception as e:
            raise Exception("run() Exception in NonRemotePythonTest:" + str(e))


# Equivalent to run_aws_tests.sh
class RemoteTest(AbstractTest):
    """
    A class to handle remote C++ tests.

    This class provides methods to set up the environment and run remote C++
    tests. It extends the AbstractTest class.
    """

    def fill_default_arguments(self, testingArgs: TestingArgs) -> TestingArgs:
        """
        Fills in default arguments for the RemoteTest object.

        This method fills in default arguments for the RemoteTest object based
        on the specified test name. It sets default values for test name filters,
        MinIO configurations, Google test values, and VDMS configurations. It
        also handles exceptions.

        Parameters:
        - testingArgs (TestingArgs): The arguments provided by the user.

        Returns:
        - TestingArgs: The arguments with default values filled in.

        Raises:
        - Exception: If any error occurs during the process of filling default
        arguments.
        """
        try:
            # Using the flag "-n YOUR_TEST_NAME" for specifying the test name filter.
            # In case that this flag is not specified then it will use the default
            # filter pattern
            testingArgs.test_name = self.get_valid_test_name(
                testingArgs, DEFAULT_REMOTE_UNIT_TEST_FILTER
            )

            # Fill the default required values for MinIO
            testingArgs = self.get_valid_minio_values(testingArgs)

            # Fill the default values for running the googletest tests
            testingArgs = self.get_valid_google_test_values(testingArgs)

            # Use the flag "-c CONFIG_FILEPATH_1 -c CONFIG_FILEPATH_2"
            # In case this flag is not specified then it will use the following config
            # files as default
            # For "ru" tests: 'unit_tests/config-aws-tests.json'
            testingArgs.vdms_app_path, testingArgs.config_files_for_vdms = (
                self.get_valid_vdms_values(
                    testingArgs, DEFAULT_REMOTE_UNIT_TEST_CONFIG_FILES
                )
            )

        except Exception as e:
            raise Exception("fill_default_arguments() in RemoteTest() error: " + str(e))

        return testingArgs

    def validate_arguments(
        self, testingArgs: TestingArgs, parser: argparse.ArgumentParser
    ):
        """
        Validates the arguments provided by the user for remote tests.

        This method performs validation for the MinIO arguments and the path to
        the binary for running Google tests. It also handles exceptions.

        Parameters:
        - testingArgs (TestingArgs): The arguments provided by the user.
        - parser (argparse.ArgumentParser): The argument parser.

        Raises:
        - Exception: If the parser is None or if any validation fails.
        """
        try:
            if parser is None:
                raise Exception("parser is None")

            # When the -t argument is for remote tests then MinIO credentials must be specified
            # Validate MinIO arguments
            self.validate_minio_values(testingArgs, parser)

            # Validate the path to the binary in charge of running the googletest tests
            self.validate_google_test_path(testingArgs, parser)

        except Exception as e:
            raise Exception("validate_arguments() in RemoteTest error: " + str(e))

    def run(self, testingArgs: TestingArgs):
        """
        Executes the remote tests based on the provided arguments.

        This method opens log files, starts the MinIO server, and runs the Google
        tests based on the specified test name. It also handles exceptions and
        prints debug information if DEBUG_MODE is enabled.

        Parameters:
        - testingArgs (TestingArgs): The arguments provided by the user.

        Raises:
        - Exception: If any error occurs during the execution of the tests.
        """
        testsStderrFD = ""
        testsStdoutFD = ""
        minioStderrFD = ""
        minioStdoutFD = ""


        if DEBUG_MODE:
            print("RemoteTest::run() was called")

        try:
            # Open the log files
            testsStderrFD, testsStdoutFD = self.open_log_files(
                testingArgs.tmp_tests_dir,
                testingArgs.stderr_filename,
                testingArgs.stdout_filename,
            )

            minioStderrFD, minioStdoutFD = self.open_log_files(
                testingArgs.tmp_tests_dir,
                testingArgs.minio_stderr_filename,
                testingArgs.minio_stdout_filename,
            )

            if DEBUG_MODE:
                print("RemoteTest::open_log_files() were called")

            # Start the MinIO Server and create the bucket
            self.run_minio_server(testingArgs, minioStderrFD, minioStdoutFD)

            if DEBUG_MODE:
                print("RemoteTest::run_minio_server() was called")

            # If the argument "run" is True then it executes the tests
            # (By default the value is True)
            if hasattr(testingArgs, "run") and testingArgs.run:
                # Run the Googletest tests
                if DEBUG_MODE:
                    print("RemoteTest::run_google_tests() was called")
                self.run_google_tests(testingArgs, testsStderrFD, testsStdoutFD)

            print("Finished")
        except Exception as e:
            raise Exception("run() Exception in RemoteTest:" + str(e))


# Equivalent to run_python_aws_tests.sh
class RemotePythonTest(AbstractTest):
    """
    A class to handle remote Python tests.

    This class provides methods to set up the environment and run remote
    Python tests. It extends the AbstractTest class.
    """

    def set_env_var_for_skipping_python_tests(self):
        """
        Sets the environment variable to skip specific Python tests.

        This method sets the 'VDMS_SKIP_REMOTE_PYTHON_TESTS' environment variable
        to True in order to skip Python tests that are specific to non-remote
        tests. It also prints debug information if DEBUG_MODE is enabled.

        Global Variables:
        - DEBUG_MODE (bool): A global flag indicating whether debug information
        should be printed.
        """
        # There are some Python tests which have to be skipped as they are specific
        # for NON Remote tests, in order to do that the
        # 'VDMS_SKIP_REMOTE_PYTHON_TESTS' environment variable must be set to True
        if DEBUG_MODE:
            print("Setting to True the VDMS_SKIP_REMOTE_PYTHON_TESTS env var")
        os.environ["VDMS_SKIP_REMOTE_PYTHON_TESTS"] = "True"

    def fill_default_arguments(self, testingArgs: TestingArgs) -> TestingArgs:
        """
        Fills in default arguments for the RemotePythonTest object.

        This method fills in default arguments for the RemotePythonTest object
        based on the specified test name. It sets default values for test name
        filters, MinIO configurations, and VDMS configurations.
        It also handles exceptions.

        Parameters:
        - testingArgs (TestingArgs): The arguments provided by the user.

        Returns:
        - TestingArgs: The arguments with default values filled in.

        Raises:
        - Exception: If any error occurs during the process of filling default
        arguments.
        """
        try:

            # Using the flag "-n YOUR_TEST_NAME" for specifying the test name filter.
            # In case that this flag is not specified then it will use the default
            # filter pattern
            testingArgs.test_name = self.get_valid_test_name(
                testingArgs, DEFAULT_REMOTE_PYTHON_TEST_FILTER
            )

            # Fill default values for MinIO
            testingArgs = self.get_valid_minio_values(testingArgs)

            # Use the flag "-c CONFIG_FILEPATH_1 -c CONFIG_FILEPATH_2"
            # Note: It will create an instance of VDMS server per each occurrence of
            # -c with its corresponding path to the config file.
            # In case this flag is not specified then it will use the following config
            # files as default
            # For "rp" tests: python/config-aws-tests.json
            testingArgs.vdms_app_path, testingArgs.config_files_for_vdms = (
                self.get_valid_vdms_values(
                    testingArgs, DEFAULT_REMOTE_PYTHON_CONFIG_FILES
                )
            )

        except Exception as e:
            raise Exception(
                "fill_default_arguments() in RemotePythonTest() error: " + str(e)
            )

        return testingArgs

    def validate_arguments(
        self, testingArgs: TestingArgs, parser: argparse.ArgumentParser
    ):
        """
        Validates the arguments provided by the user for remote Python tests.

        This method performs validation for the MinIO arguments, VDMS arguments,
        and the path to the binary for running Google tests. It also handles
        exceptions.

        Parameters:
        - testingArgs (TestingArgs): The arguments provided by the user.
        - parser (argparse.ArgumentParser): The argument parser.

        Raises:
        - Exception: If the parser is None or if any validation fails.
        """
        try:
            if parser is None:
                raise Exception("parser is None")

            # When the -t argument is for remote tests then MinIO credentials must be specified

            # Validate MinIO arguments
            self.validate_minio_values(testingArgs, parser)

            # Validate the VDMS arguments
            self.validate_vdms_values(testingArgs, parser)

            # Validate the path to the binary in charge of running the googletest tests
            self.validate_google_test_path(testingArgs, parser)

        except Exception as e:
            raise Exception("validate_arguments() in RemotePythonTest error: " + str(e))

    def run(self, testingArgs: TestingArgs):
        """
        Executes the remote Python tests based on the provided arguments.

        This method sets the values for the Python client, opens log files,
        prepares the TLS environment, starts the VDMS server, starts the MinIO
        server, sets the environment variable for skipping specific Python tests,
        and runs the Python tests based on the specified test name. It also
        handles exceptions and prints debug information if DEBUG_MODE is enabled.

        Parameters:
        - testingArgs (TestingArgs): The arguments provided by the user.

        Raises:
        - Exception: If any error occurs during the execution of the tests.
        """
        testsStderrFD = ""
        testsStdoutFD = ""
        tlsStderrFD = ""
        tlsStdoutFD = ""
        vdmsStderrFD = ""
        vdmsStdoutFD = ""
        minioStderrFD = ""
        minioStdoutFD = ""

        try:
            # Set the values for the Python client
            self.set_values_for_python_client()

            # Open the log files
            testsStderrFD, testsStdoutFD = self.open_log_files(
                testingArgs.tmp_tests_dir,
                testingArgs.stderr_filename,
                testingArgs.stdout_filename,
            )

            tlsStderrFD, tlsStdoutFD = self.open_log_files(
                testingArgs.tmp_tests_dir,
                testingArgs.tls_stderr_filename,
                testingArgs.tls_stdout_filename,
            )

            vdmsStderrFD, vdmsStdoutFD = self.open_log_files(
                testingArgs.tmp_tests_dir,
                testingArgs.vdms_stderr_filename,
                testingArgs.vdms_stdout_filename,
            )

            minioStderrFD, minioStdoutFD = self.open_log_files(
                testingArgs.tmp_tests_dir,
                testingArgs.minio_stderr_filename,
                testingArgs.minio_stdout_filename,
            )

            # Prepare the TLS environment for testing
            self.run_prep_certs_script(tlsStderrFD, tlsStdoutFD)

            # Start an instance of the VDMS server per each config file given as argument
            self.run_vdms_server(testingArgs, vdmsStderrFD, vdmsStdoutFD)

            # Start the MinIO Server and create the bucket
            self.run_minio_server(testingArgs, minioStderrFD, minioStdoutFD)

            self.set_env_var_for_skipping_python_tests()

            # If the argument "run" is True then it executes the tests
            # (By default the value is True)
            if hasattr(testingArgs, "run") and testingArgs.run:
                # Run the Python tests
                self.run_python_tests(testingArgs, testsStderrFD, testsStdoutFD)

            print("Finished")

        except Exception as e:
            raise Exception("run() Exception in RemotePythonTest: " + str(e))


class TestingParser:
    """
    A class to handle the parsing of testing arguments.

    This class provides a parser for command-line arguments used in testing.
    It uses the argparse library to define and parse the arguments.

    Attributes:
    - parser (argparse.ArgumentParser): The argument parser.
    """

    parser: argparse.ArgumentParser

    def __init__(self):
        """
        Initializes the TestingParser instance.

        This method initializes the TestingParser instance by setting the parser
        attribute to None.

        Attributes:
        - parser (argparse.ArgumentParser or None): The argument parser, initially
        set to None.
        """
        self.parser = None

    def parse_arguments(self):
        """
        Parses command-line arguments for running tests.

        This method sets up an argument parser with various options for running
        tests, including paths, ports, usernames, passwords, and other test
        configurations. It also handles exceptions.

        Returns:
        - Namespace: The parsed arguments.

        Raises:
        - Exception: If any error occurs during the parsing of arguments.
        """
        if DEBUG_MODE:
            print("parse_arguments() was called")

        try:
            self.parser = argparse.ArgumentParser(
                description="Run all the tests according to the arguments given"
            )
            self.parser.add_argument(
                "-j",
                "--json",
                type=str,
                help="Path to the JSON config file where all the argument values can be found",
                metavar="PATH",
            )
            self.parser.add_argument(
                "-a",
                "--minio_port",
                type=int,
                help="The number of port to connect to the MinIO server",
                metavar="PORT",
            )
            self.parser.add_argument(
                "-c",
                "--config_files_for_vdms",
                type=str,
                help="Create a list of config files to be used by VDMS instances",
                action="append",
                metavar="PATH",
            )
            self.parser.add_argument(
                "-d",
                "--tmp_tests_dir",
                type=str,
                help="Temporary dir for the files/dirs created by the tests",
                metavar="PATH",
            )
            self.parser.add_argument(
                "-e",
                "--stderr_filename",
                type=str,
                help="Name of the file where the stderr messages are going to be written to",
                metavar="PATH",
            )
            self.parser.add_argument(
                "-g",
                "--googletest_path",
                type=str,
                help="Path to the compiled binary of the tests used by googletest",
                metavar="PATH",
            )
            self.parser.add_argument(
                "-k",
                "--keep_tmp_tests_dir",
                action="store_true",
                default=False,
                help="If True then it does not delete the temporary directory created for the tests",
            )
            self.parser.add_argument(
                "-m",
                "--minio_app_path",
                type=str,
                help="Path to the MinIO server app",
                metavar="PATH",
            )
            self.parser.add_argument(
                "-n",
                "--test_name",
                type=str,
                help="The name of the test or the pattern of the test names",
            )
            self.parser.add_argument(
                "-o",
                "--stdout_filename",
                type=str,
                help="Name of the file where the stdout messages are going to be written to",
                metavar="PATH",
            )
            self.parser.add_argument(
                "-p",
                "--minio_password",
                type=str,
                help="The password used to connect to the MinIO server",
            )
            self.parser.add_argument(
                "-s",
                "--stop_tests_on_failure",
                action="store_true",
                default=False,
                help="If True then, the execution of the rest of the tests will be skipped when one test fails (available only for googletest)",
            )
            self.parser.add_argument(
                "-t",
                "--type_of_test",
                type=str,
                help="The type of the test: 'ut' for non remote unit tests, 'ru' for remote unit tests, 'pt' for non remote Python tests, 'rp' for remote Python tests and 'neo' for Neo4j tests",
                choices=TYPE_OF_TESTS_AVAILABLE,
            )
            self.parser.add_argument(
                "-u",
                "--minio_username",
                type=str,
                help="The username used to connect to the MinIO server",
            )
            self.parser.add_argument(
                "-v",
                "--vdms_app_path",
                type=str,
                help="The path to the VDMS app",
                metavar="PATH",
            )
            self.parser.add_argument(
                "-y",
                "--minio_console_port",
                type=int,
                help="Console Port for Minio server",
                metavar="PORT",
            )
            self.parser.add_argument(
                "-r",
                "--neo4j_port",
                type=int,
                help="Port for Neo4j container",
                metavar="PORT",
            )
            self.parser.add_argument(
                "-w",
                "--neo4j_password",
                type=str,
                help="Password for Neo4j container",
                metavar="PASSWORD",
            )
            self.parser.add_argument(
                "-x",
                "--neo4j_username",
                type=str,
                help="Username for Neo4j container",
                metavar="USERNAME",
            )
            self.parser.add_argument(
                "-z",
                "--neo4j_endpoint",
                type=str,
                help="Endpoint for Neo4j container",
                metavar="ENDPOINT",
            )
            self.parser.add_argument(
                "-b",
                "--run",
                action="store_false",
                default=True,
                help="If False then it validates the arguments but it doesn't run the tests",
            )

            args = self.parser.parse_args()
            return args
        except Exception as e:
            raise Exception("parse_arguments() error: " + str(e))

    def get_parser(self) -> argparse.ArgumentParser:
        """
        Returns the current argument parser.

        This method returns the current instance of the argument parser.

        Returns:
        - argparse.ArgumentParser: The current argument parser.
        """
        if DEBUG_MODE:
            print("get_parser() was called")
        return self.parser

    def set_parser(self, newParser: argparse.ArgumentParser):
        """
        Sets a new argument parser.

        This method sets a new instance of the argument parser.

        Parameters:
        - newParser (argparse.ArgumentParser): The new argument parser to set.
        """
        self.parser = newParser

    def read_json_config_file(
        self, jsonConfigPath: str, parser: argparse.ArgumentParser
    ) -> TestingArgs:
        """
        Reads a JSON config file and converts it to TestingArgs.

        This method reads a JSON config file from the specified path, validates
        its existence, and converts its contents to a TestingArgs object. It also
        handles exceptions.

        Parameters:
        - jsonConfigPath (str): The path to the JSON config file.
        - parser (argparse.ArgumentParser): The argument parser.

        Returns:
        - TestingArgs: The arguments converted from the JSON config file.

        Raises:
        - Exception: If the parser is None, the file does not exist, or any error
        occurs during the reading and conversion process.
        """
        if DEBUG_MODE:
            print("read_json_config_file() was called")

        try:
            if parser is None:
                raise Exception("parser is None")

            if not os.path.exists(jsonConfigPath):
                parser.error(
                    jsonConfigPath + " does not exist or there is not access to it"
                )
            else:
                with open(jsonConfigPath, "r") as file:
                    jsonConfigData = json.loads(file.read())
                    testingArgs = self.convert_json_to_testing_args(
                        jsonConfigData, parser
                    )
                    file.close()
                    return testingArgs
        except Exception as e:
            raise Exception("read_json_config_file error: " + str(e))

    def convert_json_to_testing_args(
        self, jsonConfigData: dict, parser: argparse.ArgumentParser
    ) -> TestingArgs:
        """
        Converts JSON config data to TestingArgs.

        This method validates and converts JSON config data to a TestingArgs
        object. It checks the data types of various fields and sets the
        corresponding attributes in the TestingArgs object. It also handles
        exceptions.

        Parameters:
        - jsonConfigData (dict): The JSON config data.
        - parser (argparse.ArgumentParser): The argument parser.

        Returns:
        - TestingArgs: The arguments converted from the JSON config data.

        Raises:
        - Exception: If the parser is None, or if any validation fails.
        """
        testingArgs = TestingArgs()
        try:
            if parser is None:
                raise Exception("parser is None")

            # Validations according to the expected datatype
            if jsonConfigData.get("vdms_app_path") is not None and not isinstance(
                jsonConfigData["vdms_app_path"], str
            ):
                raise Exception(
                    "'vdms_app_path' value in the JSON file is not a valid string"
                )

            if jsonConfigData.get("googletest_path") is not None and not isinstance(
                jsonConfigData["googletest_path"], str
            ):
                raise Exception(
                    "'googletest_path' value in the JSON file is not a valid string"
                )

            if jsonConfigData.get("test_name") is not None and not isinstance(
                jsonConfigData["test_name"], str
            ):
                raise Exception(
                    "'test_name' value in the JSON file is not a valid string"
                )

            if jsonConfigData.get("minio_username") is not None and not isinstance(
                jsonConfigData["minio_username"], str
            ):
                raise Exception(
                    "'minio_username' value in the JSON file is not a valid string"
                )

            if jsonConfigData.get("minio_password") is not None and not isinstance(
                jsonConfigData["minio_password"], str
            ):
                raise Exception(
                    "'minio_password' value in the JSON file is not a valid string"
                )

            if jsonConfigData.get("type_of_test") is not None and not isinstance(
                jsonConfigData["type_of_test"], str
            ):
                raise Exception(
                    "'type_of_test' value in the JSON file is not a valid string"
                )

            if jsonConfigData.get("tmp_tests_dir") is not None and not isinstance(
                jsonConfigData["tmp_tests_dir"], str
            ):
                raise Exception(
                    "'tmp_tests_dir' value in the JSON file is not a valid string"
                )

            if jsonConfigData.get("stderr_filename") is not None and not isinstance(
                jsonConfigData["stderr_filename"], str
            ):
                raise Exception(
                    "'stderr_filename' value in the JSON file is not a valid string"
                )

            if jsonConfigData.get("stdout_filename") is not None and not isinstance(
                jsonConfigData["stdout_filename"], str
            ):
                raise Exception(
                    "'stdout_filename' value in the JSON file is not a valid string"
                )

            if jsonConfigData.get(
                "config_files_for_vdms"
            ) is not None and not isinstance(
                jsonConfigData["config_files_for_vdms"], list
            ):
                raise Exception(
                    "'config_files_for_vdms' value in the JSON file is not a valid list"
                )

            if jsonConfigData.get("minio_app_path") is not None and not isinstance(
                jsonConfigData["minio_app_path"], str
            ):
                raise Exception(
                    "'minio_app_path' value in the JSON file is not a valid string"
                )

            if jsonConfigData.get("keep_tmp_tests_dir") is not None and not isinstance(
                jsonConfigData["keep_tmp_tests_dir"], bool
            ):
                raise Exception(
                    "'keep_tmp_tests_dir' value in the JSON file is not a valid bool"
                )

            if jsonConfigData.get(
                "stop_tests_on_failure"
            ) is not None and not isinstance(
                jsonConfigData["stop_tests_on_failure"], bool
            ):
                raise Exception(
                    "'stop_tests_on_failure' value in the JSON file is not a valid bool"
                )

            if jsonConfigData.get("minio_port") is not None and not isinstance(
                jsonConfigData["minio_port"], int
            ):
                raise Exception(
                    "'minio_port' value in the JSON file is not a valid integer"
                )

            if jsonConfigData.get("minio_console_port") is not None and not isinstance(
                jsonConfigData["minio_console_port"], int
            ):
                raise Exception(
                    "'minio_console_port' value in the JSON file is not a valid integer"
                )

            if jsonConfigData.get("neo4j_port") is not None and not isinstance(
                jsonConfigData["neo4j_port"], int
            ):
                raise Exception(
                    "'neo4j_port' value in the JSON file is not a valid integer"
                )

            if jsonConfigData.get("neo4j_password") is not None and not isinstance(
                jsonConfigData["neo4j_password"], str
            ):
                raise Exception(
                    "'neo4j_password' value in the JSON file is not a valid string"
                )

            if jsonConfigData.get("neo4j_username") is not None and not isinstance(
                jsonConfigData["neo4j_username"], str
            ):
                raise Exception(
                    "'neo4j_username' value in the JSON file is not a valid string"
                )

            if jsonConfigData.get("neo4j_endpoint") is not None and not isinstance(
                jsonConfigData["neo4j_endpoint"], str
            ):
                raise Exception(
                    "'neo4j_endpoint' value in the JSON file is not a valid string"
                )

            if jsonConfigData.get("run") is not None and not isinstance(
                jsonConfigData["run"], bool
            ):
                raise Exception("'run' value in the JSON file is not a valid bool")

            # Setting the values if they are valid
            if jsonConfigData.get("vdms_app_path") is not None:
                testingArgs.vdms_app_path = jsonConfigData["vdms_app_path"]

            if jsonConfigData.get("googletest_path") is not None:
                testingArgs.googletest_path = jsonConfigData["googletest_path"]

            if jsonConfigData.get("test_name") is not None:
                testingArgs.test_name = jsonConfigData["test_name"]

            if jsonConfigData.get("minio_username") is not None:
                testingArgs.minio_username = jsonConfigData["minio_username"]

            if jsonConfigData.get("minio_password") is not None:
                testingArgs.minio_password = jsonConfigData["minio_password"]

            if jsonConfigData.get("type_of_test") is not None:
                testingArgs.type_of_test = jsonConfigData["type_of_test"]

            if jsonConfigData.get("tmp_tests_dir") is not None:
                testingArgs.tmp_tests_dir = jsonConfigData["tmp_tests_dir"]

            if jsonConfigData.get("stderr_filename") is not None:
                testingArgs.stderr_filename = jsonConfigData["stderr_filename"]

            if jsonConfigData.get("stdout_filename") is not None:
                testingArgs.stdout_filename = jsonConfigData["stdout_filename"]

            if jsonConfigData.get("config_files_for_vdms") is not None:
                testingArgs.config_files_for_vdms = jsonConfigData[
                    "config_files_for_vdms"
                ]

            if jsonConfigData.get("minio_app_path") is not None:
                testingArgs.minio_app_path = jsonConfigData["minio_app_path"]

            if jsonConfigData.get("minio_port") is not None:
                testingArgs.minio_port = jsonConfigData["minio_port"]

            if jsonConfigData.get("stop_tests_on_failure") is not None:
                testingArgs.stop_tests_on_failure = bool(
                    jsonConfigData["stop_tests_on_failure"]
                )

            if jsonConfigData.get("keep_tmp_tests_dir") is not None:
                testingArgs.keep_tmp_tests_dir = bool(
                    jsonConfigData["keep_tmp_tests_dir"]
                )

            if jsonConfigData.get("minio_console_port") is not None:
                testingArgs.minio_console_port = jsonConfigData["minio_console_port"]

            if jsonConfigData.get("neo4j_port") is not None:
                testingArgs.neo4j_port = jsonConfigData["neo4j_port"]

            if jsonConfigData.get("neo4j_password") is not None:
                testingArgs.neo4j_password = jsonConfigData["neo4j_password"]

            if jsonConfigData.get("neo4j_username") is not None:
                testingArgs.neo4j_username = jsonConfigData["neo4j_username"]

            if jsonConfigData.get("neo4j_endpoint") is not None:
                testingArgs.neo4j_endpoint = jsonConfigData["neo4j_endpoint"]

            if jsonConfigData.get("run") is not None:
                testingArgs.run = bool(jsonConfigData["run"])

        except Exception as e:
            if parser is not None:
                parser.error("convert_json_to_testing_args error: " + str(e))
            else:
                raise Exception("convert_json_to_testing_args error: " + str(e))

        return testingArgs

    def convert_namespace_to_testing_args(
        self, namespaceData, parser: argparse.ArgumentParser
    ):
        """
        Converts namespace data to TestingArgs.

        This method validates and converts namespace data to a TestingArgs object.
        It checks the data types of various fields and sets the corresponding
        attributes in the TestingArgs object. It also handles exceptions.

        Parameters:
        - namespaceData: The namespace data.
        - parser (argparse.ArgumentParser): The argument parser.

        Returns:
        - TestingArgs: The arguments converted from the namespace data.

        Raises:
        - Exception: If the parser is None, or if any validation fails.
        """
        if DEBUG_MODE:
            print("convert_namespace_to_testing_args() was called")

        testingArgs = TestingArgs()
        try:
            if parser is None:
                raise Exception("parser is None")

            testingArgs.vdms_app_path = namespaceData.vdms_app_path
            testingArgs.googletest_path = namespaceData.googletest_path
            testingArgs.test_name = namespaceData.test_name
            testingArgs.minio_username = namespaceData.minio_username
            testingArgs.minio_password = namespaceData.minio_password
            testingArgs.type_of_test = namespaceData.type_of_test
            testingArgs.tmp_tests_dir = namespaceData.tmp_tests_dir
            testingArgs.stderr_filename = namespaceData.stderr_filename
            testingArgs.stdout_filename = namespaceData.stdout_filename

            if (
                hasattr(namespaceData, "config_files_for_vdms")
                and namespaceData.config_files_for_vdms is not None
                and not isinstance(namespaceData.config_files_for_vdms, list)
            ):
                raise Exception(
                    "'config_files_for_vdms' value in the namespace is not a valid list"
                )
            testingArgs.config_files_for_vdms = namespaceData.config_files_for_vdms

            testingArgs.minio_app_path = namespaceData.minio_app_path
            testingArgs.minio_port = namespaceData.minio_port

            if (
                hasattr(namespaceData, "stop_tests_on_failure")
                and namespaceData.stop_tests_on_failure is not None
                and not isinstance(namespaceData.stop_tests_on_failure, bool)
            ):
                raise Exception(
                    "'stop_tests_on_failure' value in the namespace is not a valid bool"
                )
            testingArgs.stop_tests_on_failure = namespaceData.stop_tests_on_failure

            if (
                hasattr(namespaceData, "keep_tmp_tests_dir")
                and namespaceData.keep_tmp_tests_dir is not None
                and not isinstance(namespaceData.keep_tmp_tests_dir, bool)
            ):
                raise Exception(
                    "'keep_tmp_tests_dir' value in the namespace is not a valid bool"
                )
            testingArgs.keep_tmp_tests_dir = namespaceData.keep_tmp_tests_dir

            testingArgs.minio_console_port = namespaceData.minio_console_port
            testingArgs.neo4j_port = namespaceData.neo4j_port
            testingArgs.neo4j_password = namespaceData.neo4j_password
            testingArgs.neo4j_username = namespaceData.neo4j_username
            testingArgs.neo4j_endpoint = namespaceData.neo4j_endpoint

            if (
                hasattr(namespaceData, "run")
                and namespaceData.run is not None
                and not isinstance(namespaceData.run, bool)
            ):
                raise Exception("'run' value in the namespace is not a valid bool")
            testingArgs.run = namespaceData.run
        except Exception as e:
            if parser is not None:
                parser.error("convert_namespace_to_testing_args() error: " + str(e))
            else:
                raise Exception("convert_namespace_to_testing_args() error: " + str(e))
        return testingArgs

    def validate_type_test_value(
        self, testingArgs: TestingArgs, parser: argparse.ArgumentParser
    ):
        """
        Validates the type_of_test argument.

        This method checks if the `type_of_test` attribute in `testingArgs` is
        set. If not, it raises a parser error indicating that the argument is
        required.

        Parameters:
        - testingArgs (TestingArgs): The arguments provided by the user.
        - parser (argparse.ArgumentParser): The argument parser.

        Raises:
        - Exception: If the `type_of_test` attribute is not set or is invalid.
        """
        # Validate -t (type_of_test) argument is set
        if (
            not hasattr(testingArgs, "type_of_test")
            or testingArgs.type_of_test is None
            or testingArgs.type_of_test == ""
        ):
            parser.error(
                "the following argument is required: -t/--type_of_test "
                + str(TYPE_OF_TESTS_AVAILABLE)
            )

    def validate_stop_testing_value(self, testingArgs: TestingArgs):
        """
        Validates the stop_tests_on_failure argument.

        This method checks if the `stop_tests_on_failure` attribute in
        `testingArgs` is set. If it is set to True, it verifies that the
        `type_of_test` is one of the Googletest types. If not, it prints a warning
        message indicating that the flag will be ignored.

        Parameters:
        - testingArgs (TestingArgs): The arguments provided by the user.
        """
        # Using the flag "-s (stop testing on failure)"
        # for specifying if google test has to stop the execution when
        # there is a failure in one of the tests
        if (
            hasattr(testingArgs, "stop_tests_on_failure")
            and testingArgs.stop_tests_on_failure is not None
        ):
            if testingArgs.stop_tests_on_failure:
                if testingArgs.type_of_test not in GOOGLETEST_TYPE_OF_TESTS:
                    warningMessage = (
                        "stop_tests_on_failure flag is only used by Googletest tests.\n"
                    )
                    warningMessage += "This flag will be ignored"
                    print(warningMessage)

    def validate_common_arguments(
        self, testingArgs: TestingArgs, parser: argparse.ArgumentParser
    ):
        """
        Validates common arguments for testing.

        This method performs common validations for testing arguments. It checks
        if the parser is provided, validates the `type_of_test` argument, and
        validates the `stop_tests_on_failure` argument.

        Parameters:
        - testingArgs (TestingArgs): The arguments provided by the user.
        - parser (argparse.ArgumentParser): The argument parser.

        Raises:
        - Exception: If the parser is None or if any validation fails.
        """

        if parser is None:
            raise Exception("parser is None")

        # Validate type of test was set
        self.validate_type_test_value(testingArgs, parser)

        # Validate the -s flag
        self.validate_stop_testing_value(testingArgs)

    def validate_arguments(
        self, testingArgs: TestingArgs, parser: argparse.ArgumentParser
    ):
        """
        Validates the arguments provided by the user for different types of tests.

        This method performs common validations and then delegates specific
        validations based on the `type_of_test` argument. It also handles
        exceptions.

        Parameters:
        - testingArgs (TestingArgs): The arguments provided by the user.
        - parser (argparse.ArgumentParser): The argument parser.

        Raises:
        - Exception: If any validation fails.
        """
        if DEBUG_MODE:
            print("TestingParser::validateArguments() was called")

        try:
            self.validate_common_arguments(testingArgs, parser)

            if testingArgs.type_of_test == "pt":
                tests = NonRemotePythonTest()
                tests.validate_arguments(testingArgs, parser)
            elif testingArgs.type_of_test == "rp":
                tests = RemotePythonTest()
                tests.validate_arguments(testingArgs, parser)
            elif testingArgs.type_of_test == "ut":
                tests = NonRemoteTest()
                tests.validate_arguments(testingArgs, parser)
            elif testingArgs.type_of_test == "neo":
                tests = Neo4jTest()
                tests.validate_arguments(testingArgs, parser)
            elif testingArgs.type_of_test == "ru":
                tests = RemoteTest()
                tests.validate_arguments(testingArgs, parser)
        except Exception as e:
            raise Exception("validate_arguments() error: " + str(e))

    def fill_default_stop_testing_value(self, testingArgs: TestingArgs):
        """
        Fills the default value for the stop_tests_on_failure argument.

        This method sets the default value for the `stop_tests_on_failure`
        attribute in `testingArgs` based on the `type_of_test`. If the test type
        is one of the Googletest types, it sets the default value to False if not
        already set. If the test type is not a Googletest type, it sets the value
        to None.

        Parameters:
        - testingArgs (TestingArgs): The arguments provided by the user.

        Returns:
        - TestingArgs: The arguments with the default value for
        `stop_tests_on_failure` filled in.
        """

        # Using the flag "-s"
        # for specifying if google test has to stop the execution when
        # there is a failure in one of the tests
        if testingArgs.type_of_test in GOOGLETEST_TYPE_OF_TESTS:
            if (
                not hasattr(testingArgs, "stop_tests_on_failure")
                or testingArgs.stop_tests_on_failure is None
            ):
                testingArgs.stop_tests_on_failure = False

            if testingArgs.stop_tests_on_failure:
                print("Using", STOP_ON_FAILURE_FLAG)
        else:
            testingArgs.stop_tests_on_failure = None

        return testingArgs

    def _set_default_if_unset(
        self, testing_args: TestingArgs, attribute_name: str, default_value
    ):
        """
        Sets a default value for an attribute of testing_args if it is not already set.

        This method checks if a specified attribute of the `testing_args` object
        is set. If the attribute is not set (i.e., it is None or an empty string),
        it sets the attribute to a provided default value. It also prints debug
        information if DEBUG_MODE is enabled.

        Parameters:
        - testing_args (TestingArgs): The TestingArgs object to update.
        - attribute_name (str): The name of the attribute to check and possibly set.
        - default_value: The default value to set if the attribute is not set.
        """

        if not hasattr(testing_args, attribute_name) or getattr(
            testing_args, attribute_name
        ) in (None, ""):
            setattr(testing_args, attribute_name, default_value)
            if DEBUG_MODE:
                print(f"Using default {attribute_name}: {default_value}")

    def fill_default_tmp_dir_value(self, testing_args: TestingArgs):
        """
        Sets the default temporary directory for testing if it is not already
        specified.

        This method checks if the `tmp_tests_dir` attribute in the `testing_args`
        object is set. If it is not set, it assigns a default value to it. The
        default value is specified by the constant `DEFAULT_TMP_DIR`. The method
        also updates a global variable `global_tmp_tests_dir` with the value of
        `tmp_tests_dir`.

        Parameters:
        - testing_args (TestingArgs): An instance of the `TestingArgs` class
            which contains various arguments and configurations for testing.
            This object should have an attribute `tmp_tests_dir`.

        Returns:
        - TestingArgs: The updated `testing_args` object with the `tmp_tests_dir`
            attribute set to the default value if it was not already specified.

        Notes:
            - The method uses a global variable `global_tmp_tests_dir` to store
            the value of `tmp_tests_dir` for global access.
            - The `tmp_tests_dir` attribute specifies the directory where MinIO
            and temporary files or directories should be created.
            - If the `tmp_tests_dir` attribute is not specified in the
            `testing_args` object, the method assigns a default directory
            `DEFAULT_TMP_DIR` to it.
            - The directory can be specified using the flag
            "-d TEMPORARY_OUTPUT_DIRECTORY".
        """

        global global_tmp_tests_dir
        # Use the flag "-d TEMPORARY_OUTPUT_DIRECTORY"
        # for specifying the directory where the minio and temporary files or dirs
        # should be created. In case that this flag is not specified
        # then it will use the default "tests_output_dir" directory
        self._set_default_if_unset(testing_args, "tmp_tests_dir", DEFAULT_TMP_DIR)
        global_tmp_tests_dir = testing_args.tmp_tests_dir

        return testing_args

    def fill_default_stderr_filename(self, testing_args: TestingArgs):
        """
        Sets the default stderr filename for testing if it is not already
        specified.

        This method checks if the `stderr_filename` attribute in the
        `testing_args` object is set. If it is not set, it assigns a default
        value to it. The default value is specified by the constant
        `DEFAULT_TESTS_STDERR_FILENAME`.

        Parameters:
        - testing_args (TestingArgs):
            testing_args (TestingArgs): An instance of the `TestingArgs` class
                which contains various arguments and configurations for testing.
                This object should have an attribute `stderr_filename`.

        Returns:
        - TestingArgs: The updated `testing_args` object with the
            `stderr_filename` attribute set to the default value if it was not
            already specified.

        Notes:
            - The `stderr_filename` attribute specifies the filename where
            standard error output should be directed.
            - If the `stderr_filename` attribute is not specified in the
            `testing_args` object, the method assigns a default filename
            `DEFAULT_TESTS_STDERR_FILENAME` to it.
            - The filename can be specified using the flag "-e FILENAME".
        """

        # Default value for -e parameter
        self._set_default_if_unset(
            testing_args, "stderr_filename", DEFAULT_TESTS_STDERR_FILENAME
        )
        return testing_args

    def fill_default_stdout_filename(self, testing_args: TestingArgs):
        """
        Sets the default stdout filename for testing if it is not already
        specified.

        This method checks if the `stdout_filename` attribute in the
        `testing_args` object is set. If it is not set, it assigns a default
        value to it. The default value is specified by the constant
        `DEFAULT_TESTS_STDOUT_FILENAME`.

        Parameters:
        - testing_args (TestingArgs): An instance of the `TestingArgs` class
                which contains various arguments and configurations for testing.
                This object should have an attribute `stdout_filename`.

        Returns:
        - TestingArgs: The updated `testing_args` object with the
            `stdout_filename` attribute set to the default value if it was not
            already specified.

        Notes:
            - The `stdout_filename` attribute specifies the filename where
            standard output should be directed.
            - If the `stdout_filename` attribute is not specified in the
            `testing_args` object, the method assigns a default filename
            `DEFAULT_TESTS_STDOUT_FILENAME` to it.
            - The filename can be specified using the flag "-o FILENAME".
        """

        self._set_default_if_unset(
            testing_args, "stdout_filename", DEFAULT_TESTS_STDOUT_FILENAME
        )
        return testing_args

    def fill_default_udf_local_stderr_filename(self, testing_args: TestingArgs):
        """
        Sets the default UDF local stderr filename for testing if it is not
        already specified.

        This method checks if the `udf_local_stderr_filename` attribute in the
        `testing_args` object is set. If it is not set, it assigns a default
        value to it. The default value is specified by the constant
        `DEFAULT_UDF_LOCAL_STDERR_FILENAME`.

        Parameters:
        - testing_args (TestingArgs): An instance of the `TestingArgs` class
                which contains various arguments and configurations for testing.
                This object should have an attribute `udf_local_stderr_filename`.

        Returns:
        - TestingArgs: The updated `testing_args` object with the
            `udf_local_stderr_filename` attribute set to the default value if it
            was not already specified.

        Notes:
            - The `udf_local_stderr_filename` attribute specifies the filename
            where standard error output for UDF local operations should be
            directed.
            - If the `udf_local_stderr_filename` attribute is not specified in
            the `testing_args` object, the method assigns a default filename
            `DEFAULT_UDF_LOCAL_STDERR_FILENAME` to it.
        """
        self._set_default_if_unset(
            testing_args, "udf_local_stderr_filename", DEFAULT_UDF_LOCAL_STDERR_FILENAME
        )
        return testing_args

    def fill_default_udf_local_stdout_filename(self, testing_args: TestingArgs):
        """
        Sets the default UDF local stdout filename for testing if it is not
        already specified.

        This method checks if the `udf_local_stdout_filename` attribute in the
        `testing_args` object is set. If it is not set, it assigns a default
        value to it. The default value is specified by the constant
        `DEFAULT_UDF_LOCAL_STDOUT_FILENAME`.

        Parameters:
        - testing_args (TestingArgs): An instance of the `TestingArgs` class
                which contains various arguments and configurations for testing.
                This object should have an attribute `udf_local_stdout_filename`.

        Returns:
        - TestingArgs: The updated `testing_args` object with the
            `udf_local_stdout_filename` attribute set to the default value if it
            was not already specified.

        Notes:
            - The `udf_local_stdout_filename` attribute specifies the filename
            where standard output for UDF local operations should be directed.
            - If the `udf_local_stdout_filename` attribute is not specified in
            the `testing_args` object, the method assigns a default filename
            `DEFAULT_UDF_LOCAL_STDOUT_FILENAME` to it.
        """
        self._set_default_if_unset(
            testing_args, "udf_local_stdout_filename", DEFAULT_UDF_LOCAL_STDOUT_FILENAME
        )
        return testing_args

    def fill_default_udf_server_stderr_filename(self, testing_args: TestingArgs):
        """
        Sets the default UDF server stderr filename for testing if it is not
        already specified.

        This method checks if the `udf_server_stderr_filename` attribute in the
        `testing_args` object is set. If it is not set, it assigns a default
        value to it. The default value is specified by the constant
        `DEFAULT_UDF_SERVER_STDERR_FILENAME`.

        Parameters:
        - testing_args (TestingArgs): An instance of the `TestingArgs` class
                which contains various arguments and configurations for testing.
                This object should have an attribute `udf_server_stderr_filename`.

        Returns:
        - TestingArgs: The updated `testing_args` object with the
            `udf_server_stderr_filename` attribute set to the default value if it
            was not already specified.

        Notes:
            - The `udf_server_stderr_filename` attribute specifies the filename
            where standard error output for UDF server operations should be
            directed.
            - If the `udf_server_stderr_filename` attribute is not specified in
            the `testing_args` object, the method assigns a default filename
            `DEFAULT_UDF_SERVER_STDERR_FILENAME` to it.
        """
        self._set_default_if_unset(
            testing_args,
            "udf_server_stderr_filename",
            DEFAULT_UDF_SERVER_STDERR_FILENAME,
        )
        return testing_args

    def fill_default_udf_server_stdout_filename(self, testing_args: TestingArgs):
        """
        Sets the default UDF server stdout filename for testing if it is not
        already specified.

        This method checks if the `udf_server_stdout_filename` attribute in the
        `testing_args` object is set. If it is not set, it assigns a default
        value to it. The default value is specified by the constant
        `DEFAULT_UDF_SERVER_STDOUT_FILENAME`.

        Parameters:
        - testing_args (TestingArgs): An instance of the `TestingArgs` class
                which contains various arguments and configurations for testing.
                This object should have an attribute `udf_server_stdout_filename`.

        Returns:
        - TestingArgs: The updated `testing_args` object with the
            `udf_server_stdout_filename` attribute set to the default value if it
            was not already specified.

        Notes:
            - The `udf_server_stdout_filename` attribute specifies the filename
            where standard output for UDF server operations should be directed.
            - If the `udf_server_stdout_filename` attribute is not specified in
            the `testing_args` object, the method assigns a default filename
            `DEFAULT_UDF_SERVER_STDOUT_FILENAME` to it.
        """
        self._set_default_if_unset(
            testing_args,
            "udf_server_stdout_filename",
            DEFAULT_UDF_SERVER_STDOUT_FILENAME,
        )
        return testing_args

    def fill_default_tls_stdout_filename(self, testing_args: TestingArgs):
        """
        Sets the default TLS stdout filename for testing if it is not already
        specified.

        This method checks if the `tls_stdout_filename` attribute in the
        `testing_args` object is set. If it is not set, it assigns a default
        value to it. The default value is specified by the constant
        `DEFAULT_TLS_STDOUT_FILENAME`.

        Parameters:
        - testing_args (TestingArgs): An instance of the `TestingArgs` class
                which contains various arguments and configurations for testing.
                This object should have an attribute `tls_stdout_filename`.

        Returns:
        - TestingArgs: The updated `testing_args` object with the
            `tls_stdout_filename` attribute set to the default value if it was
            not already specified.

        Notes:
            - The `tls_stdout_filename` attribute specifies the filename where
            standard output for TLS operations should be directed.
            - If the `tls_stdout_filename` attribute is not specified in the
            `testing_args` object, the method assigns a default filename
            `DEFAULT_TLS_STDOUT_FILENAME` to it.
        """
        self._set_default_if_unset(
            testing_args,
            "tls_stdout_filename",
            DEFAULT_TLS_STDOUT_FILENAME,
        )
        return testing_args

    def fill_default_tls_stderr_filename(self, testing_args: TestingArgs):
        """
        Sets the default TLS stderr filename for testing if it is not already
        specified.

        This method checks if the `tls_stderr_filename` attribute in the
        `testing_args` object is set. If it is not set, it assigns a default
        value to it. The default value is specified by the constant
        `DEFAULT_TLS_STDERR_FILENAME`.

        Parameters:
        - testing_args (TestingArgs): An instance of the `TestingArgs` class
                which contains various arguments and configurations for testing.
                This object should have an attribute `tls_stderr_filename`.

        Returns:
        - TestingArgs: The updated `testing_args` object with the
            `tls_stderr_filename` attribute set to the default value if it was
            not already specified.

        Notes:
            - The `tls_stderr_filename` attribute specifies the filename where
            standard error output for TLS operations should be directed.
            - If the `tls_stderr_filename` attribute is not specified in the
            `testing_args` object, the method assigns a default filename
            `DEFAULT_TLS_STDERR_FILENAME` to it.
        """
        self._set_default_if_unset(
            testing_args,
            "tls_stderr_filename",
            DEFAULT_TLS_STDERR_FILENAME,
        )
        return testing_args

    def fill_default_minio_stdout_filename(self, testing_args: TestingArgs):
        """
        Sets the default MinIO stdout filename for testing if it is not already
        specified.

        This method checks if the `minio_stdout_filename` attribute in the
        `testing_args` object is set. If it is not set, it assigns a default
        value to it. The default value is specified by the constant
        `DEFAULT_MINIO_STDOUT_FILENAME`.

        Parameters:
        - testing_args (TestingArgs): An instance of the `TestingArgs` class
                which contains various arguments and configurations for testing.
                This object should have an attribute `minio_stdout_filename`.

        Returns:
        - TestingArgs: The updated `testing_args` object with the
            `minio_stdout_filename` attribute set to the default value if it was
            not already specified.

        Notes:
            - The `minio_stdout_filename` attribute specifies the filename where
            standard output for MinIO operations should be directed.
            - If the `minio_stdout_filename` attribute is not specified in the
            `testing_args` object, the method assigns a default filename
            `DEFAULT_MINIO_STDOUT_FILENAME` to it.
        """
        self._set_default_if_unset(
            testing_args,
            "minio_stdout_filename",
            DEFAULT_MINIO_STDOUT_FILENAME,
        )
        return testing_args

    def fill_default_minio_stderr_filename(self, testing_args: TestingArgs):
        """
        Sets the default MinIO stderr filename for testing if it is not already
        specified.

        This method checks if the `minio_stderr_filename` attribute in the
        `testing_args` object is set. If it is not set, it assigns a default
        value to it. The default value is specified by the constant
        `DEFAULT_MINIO_STDERR_FILENAME`.

        Parameters:
        - testing_args (TestingArgs): An instance of the `TestingArgs` class
                which contains various arguments and configurations for testing.
                This object should have an attribute `minio_stderr_filename`.

        Returns:
        - TestingArgs: The updated `testing_args` object with the
            `minio_stderr_filename` attribute set to the default value if it was
            not already specified.

        Notes:
            - The `minio_stderr_filename` attribute specifies the filename where
            standard error output for MinIO operations should be directed.
            - If the `minio_stderr_filename` attribute is not specified in the
            `testing_args` object, the method assigns a default filename
            `DEFAULT_MINIO_STDERR_FILENAME` to it.
        """
        self._set_default_if_unset(
            testing_args,
            "minio_stderr_filename",
            DEFAULT_MINIO_STDERR_FILENAME,
        )
        return testing_args

    def fill_default_vdms_stdout_filename(self, testing_args: TestingArgs):
        """
        Sets the default VDMS stdout filename for testing if it is not already
        specified.

        This method checks if the `vdms_stdout_filename` attribute in the
        `testing_args` object is set. If it is not set, it assigns a default
        value to it. The default value is specified by the constant
        `DEFAULT_VDMS_STDOUT_FILENAME`.

        Parameters:
        - testing_args (TestingArgs): An instance of the `TestingArgs` class
                which contains various arguments and configurations for testing.
                This object should have an attribute `vdms_stdout_filename`.

        Returns:
        - TestingArgs: The updated `testing_args` object with the
            `vdms_stdout_filename` attribute set to the default value if it was
            not already specified.

        Notes:
            - The `vdms_stdout_filename` attribute specifies the filename where
            standard output for VDMS operations should be directed.
            - If the `vdms_stdout_filename` attribute is not specified in the
            `testing_args` object, the method assigns a default filename
            `DEFAULT_VDMS_STDOUT_FILENAME` to it.
        """
        self._set_default_if_unset(
            testing_args,
            "vdms_stdout_filename",
            DEFAULT_VDMS_STDOUT_FILENAME,
        )
        return testing_args

    def fill_default_vdms_stderr_filename(self, testing_args: TestingArgs):
        """
        Sets the default VDMS stderr filename for testing if it is not already
        specified.

        This method checks if the `vdms_stderr_filename` attribute in the
        `testing_args` object is set. If it is not set, it assigns a default
        value to it. The default value is specified by the constant
        `DEFAULT_VDMS_STDERR_FILENAME`.

        Parameters:
        - testing_args (TestingArgs): An instance of the `TestingArgs` class
                which contains various arguments and configurations for testing.
                This object should have an attribute `vdms_stderr_filename`.

        Returns:
        - TestingArgs: The updated `testing_args` object with the
            `vdms_stderr_filename` attribute set to the default value if it was
            not already specified.

        Notes:
            - The `vdms_stderr_filename` attribute specifies the filename where
            standard error output for VDMS operations should be directed.
            - If the `vdms_stderr_filename` attribute is not specified in the
            `testing_args` object, the method assigns a default filename
            `DEFAULT_VDMS_STDERR_FILENAME` to it.
        """
        self._set_default_if_unset(
            testing_args,
            "vdms_stderr_filename",
            DEFAULT_VDMS_STDERR_FILENAME,
        )
        return testing_args

    def fill_default_keep_value(self, testingArgs: TestingArgs):
        """
        Sets the default value for keeping the temporary directory after testing
        if it is not already specified.

        This method checks if the `keep_tmp_tests_dir` attribute in the
        `testingArgs` object is set. If it is not set, it assigns a default
        value of `False` to it. The method also updates a global variable
        `global_keep_tmp_tests_dir` with the value of `keep_tmp_tests_dir`.

        Parameters:
        - testing_args (TestingArgs): An instance of the `TestingArgs` class
                which contains various arguments and configurations for testing.
                This object should have an attribute `keep_tmp_tests_dir`.

        Returns:
        - TestingArgs: The updated `testingArgs` object with the
            `keep_tmp_tests_dir` attribute set to the default value if it was
            not already specified.

        Notes:
            - The method uses a global variable `global_keep_tmp_tests_dir` to
            store the value of `keep_tmp_tests_dir` for global access.
            - The `keep_tmp_tests_dir` attribute specifies whether the temporary
            directory should be deleted once the testing is finished.
            - If the `keep_tmp_tests_dir` attribute is not specified in the
            `testingArgs` object, the method assigns a default value of `False`
            to it.
            - The flag "-k" can be used to specify this parameter.
        """
        global global_keep_tmp_tests_dir
        # Using the flag "-k"
        # for specifying if the temporary dir has to be deleted once
        # the testing finished
        # If the parameter was not set then by default it will be False
        if (
            not hasattr(testingArgs, "keep_tmp_tests_dir")
            or testingArgs.keep_tmp_tests_dir is None
        ):
            testingArgs.keep_tmp_tests_dir = False

        global_keep_tmp_tests_dir = testingArgs.keep_tmp_tests_dir
        return testingArgs

    def fill_default_run_value(self, testingArgs: TestingArgs) -> TestingArgs:
        """
        Sets the default value for running the tests if it is not already
        specified.

        This method checks if the `run` attribute in the `testingArgs` object is
        set. If it is not set, it assigns a default value of `True` to it. This
        indicates that the script should execute the validation and run the
        tests.

        Parameters:
        - testing_args (TestingArgs): An instance of the `TestingArgs` class
                which contains various arguments and configurations for testing.
                This object should have an attribute `run`.

        Returns:
        - TestingArgs: The updated `testingArgs` object with the `run`
            attribute set to the default value if it was not already specified.

        Notes:
            - The `run` attribute specifies whether the script should execute
            the validation and run the tests.
            - If the `run` attribute is not specified in the `testingArgs`
            object, the method assigns a default value of `True` to it.
            - If the `run` attribute is set to `False`, the execution of the
            tests will be skipped.
            - The flag "-b/--run" can be used to specify this parameter.
        """
        # Using the flag "-b/--run"
        # for specifying if the script must execute the validation and run the
        # tests.
        # If the parameter was not set then by default it will be True
        # If False then it will skip the execution of the tests
        if not hasattr(testingArgs, "run") or testingArgs.run is None:
            testingArgs.run = True

        return testingArgs

    def fill_common_default_arguments(self, testingArgs: TestingArgs) -> TestingArgs:
        """
        Fills common default arguments for the testing configuration if they are
        not already specified.

        This method sequentially calls various helper methods to set default
        values for several attributes in the `testingArgs` object. If any of
        these attributes are not set, the method assigns default values to them.

        Parameters:
        - testing_args (TestingArgs): An instance of the `TestingArgs` class
                which contains various arguments and configurations for testing.

        Returns:
        - TestingArgs: The updated `testingArgs` object with common default
            arguments set to their default values if they were not already
            specified.

        Raises:
            Exception: If an error occurs while filling the default arguments.

        Notes:
            - The method sets default values for the following attributes:
                - `stop_testing` (via `fill_default_stop_testing_value`)
                - `tmp_tests_dir` (via `fill_default_tmp_dir_value`)
                - `stderr_filename` (via `fill_default_stderr_filename`)
                - `stdout_filename` (via `fill_default_stdout_filename`)
                - `udf_local_stderr_filename` (via
                `fill_default_udf_local_stderr_filename`)
                - `udf_local_stdout_filename` (via
                `fill_default_udf_local_stdout_filename`)
                - `udf_server_stderr_filename` (via
                `fill_default_udf_server_stderr_filename`)
                - `udf_server_stdout_filename` (via
                `fill_default_udf_server_stdout_filename`)
                - `tls_stderr_filename` (via `fill_default_tls_stderr_filename`)
                - `tls_stdout_filename` (via `fill_default_tls_stdout_filename`)
                - `minio_stderr_filename` (via `fill_default_minio_stderr_filename`)
                - `minio_stdout_filename` (via `fill_default_minio_stdout_filename`)
                - `vdms_stderr_filename` (via `fill_default_vdms_stderr_filename`)
                - `vdms_stdout_filename` (via `fill_default_vdms_stdout_filename`)
                - `keep_tmp_tests_dir` (via `fill_default_keep_value`)
                - `run` (via `fill_default_run_value`)
        """
        try:
            # Fill default value for -s flag
            testingArgs = self.fill_default_stop_testing_value(testingArgs)

            # Fill default value for tmp directory
            testingArgs = self.fill_default_tmp_dir_value(testingArgs)

            # Default value for -e
            testingArgs = self.fill_default_stderr_filename(testingArgs)

            # Default value for -o
            testingArgs = self.fill_default_stdout_filename(testingArgs)

            # Default value for stderr log files related to udf_local
            testingArgs = self.fill_default_udf_local_stderr_filename(testingArgs)

            # Default value for stdout log files related to udf_local
            testingArgs = self.fill_default_udf_local_stdout_filename(testingArgs)

            # Default value for stderr log files related to udf_server
            testingArgs = self.fill_default_udf_server_stderr_filename(testingArgs)

            # Default value for stdout log files related to udf_server
            testingArgs = self.fill_default_udf_server_stdout_filename(testingArgs)

            # Default value for stderr log files related to tls
            testingArgs = self.fill_default_tls_stderr_filename(testingArgs)

            # Default value for stdout log files related to tls
            testingArgs = self.fill_default_tls_stdout_filename(testingArgs)

            # Default value for stderr log files related to MinIO
            testingArgs = self.fill_default_minio_stderr_filename(testingArgs)

            # Default value for stdout log files related to MinIO
            testingArgs = self.fill_default_minio_stdout_filename(testingArgs)

            # Default value for stderr log files related to VDMS
            testingArgs = self.fill_default_vdms_stderr_filename(testingArgs)

            # Default value for stdout log files related to VDMS
            testingArgs = self.fill_default_vdms_stdout_filename(testingArgs)

            # Default value for -k (keep tmp files)
            testingArgs = self.fill_default_keep_value(testingArgs)

            # Default
            testingArgs = self.fill_default_run_value(testingArgs)

            return testingArgs

        except Exception as e:
            raise Exception("fill_common_default_arguments() error: " + str(e))

    def fill_default_arguments(self, testingArgs: TestingArgs) -> TestingArgs:
        """
        Fills default arguments for the testing configuration based on the type
        of test specified.

        This method first fills common default arguments and then fills specific
        default arguments based on the `type_of_test` attribute in the
        `testingArgs` object.

        Parameters:
        - testing_args (TestingArgs): An instance of the `TestingArgs` class
                which contains various arguments and configurations for testing.

        Returns:
        - TestingArgs: The updated `testingArgs` object with default arguments
            set based on the type of test specified.

        Raises:
            Exception: If an error occurs while filling the default arguments.

        Notes:
            - The method first calls `fill_common_default_arguments` to set
            common default values.
            - It then checks the `type_of_test` attribute and calls the
            appropriate method to fill specific default arguments:
                - "pt": NonRemotePythonTest
                - "rp": RemotePythonTest
                - "ut": NonRemoteTest (C++)
                - "neo": Neo4jTest
                - "ru": RemoteTest (C++)
        """
        if DEBUG_MODE:
            print("TestingParser::fill_default_arguments() was called")

        try:
            testingArgs = self.fill_common_default_arguments(testingArgs)

            if testingArgs.type_of_test == "pt":
                tests = NonRemotePythonTest()
                testingArgs = tests.fill_default_arguments(testingArgs)
            elif testingArgs.type_of_test == "rp":
                tests = RemotePythonTest()
                testingArgs = tests.fill_default_arguments(testingArgs)
            elif testingArgs.type_of_test == "ut":
                tests = NonRemoteTest()
                testingArgs = tests.fill_default_arguments(testingArgs)
            elif testingArgs.type_of_test == "neo":
                tests = Neo4jTest()
                testingArgs = tests.fill_default_arguments(testingArgs)
            elif testingArgs.type_of_test == "ru":
                tests = RemoteTest()
                testingArgs = tests.fill_default_arguments(testingArgs)
        except Exception as e:
            raise Exception("fill_default_arguments() error: " + str(e))

        return testingArgs

    def translate_db_root_path_to_tmp_dir(self, testingArgs: TestingArgs):
        """
        Translates the `db_root_path` in VDMS config files to a temporary
        directory path.

        This method modifies the `db_root_path` attribute in the VDMS config
        files specified in `testingArgs.tmp_config_files_for_vdms` to point to a
        directory inside the temporary directory. It ensures that the config
        files are accessible and updates the `db_root_path` to a path inside
        `testingArgs.tmp_tests_dir`.

        Parameters:
        - testing_args (TestingArgs): An instance of the `TestingArgs` class
                which contains various arguments and configurations for testing.
                This object should have an attribute
                `tmp_config_files_for_vdms` which is a list of config file paths.

        Raises:
            Exception: If an error occurs while translating the `db_root_path` or
            if any of the config files do not exist or are not accessible.

        Notes:
            - The method reads each config file, removes comments, and parses the
            JSON content.
            - It updates the `db_root_path` attribute to point to a directory
            inside the temporary directory specified by
            `testingArgs.tmp_tests_dir`.
            - The modified JSON content is written back to the config file.
        """
        try:
            for tmpConfigFile in testingArgs.tmp_config_files_for_vdms:
                if not os.path.exists(tmpConfigFile):
                    raise Exception(
                        tmpConfigFile + " does not exist or there is not access to it"
                    )
                else:
                    with open(tmpConfigFile, "r") as file:
                        fileContents = file.read()
                        # Remove the comments in the JSON file
                        modifiedContents = re.sub(
                            "//.*", "", fileContents, flags=re.MULTILINE
                        )
                        jsonConfigData = json.loads(modifiedContents)
                        file.close()
                        if jsonConfigData.get("db_root_path") is not None:
                            currentDBRootPath = jsonConfigData["db_root_path"]
                            dbRootDirName = os.path.basename(currentDBRootPath)

                            jsonConfigData.update(
                                {
                                    "db_root_path": "{tmp_tests_dir}/{dbRootDirName}".format(
                                        tmp_tests_dir=testingArgs.tmp_tests_dir,
                                        dbRootDirName=dbRootDirName,
                                    )
                                }
                            )
                            modifiedJSON = json.dumps(jsonConfigData)
                            with open(tmpConfigFile, "w") as file:
                                file.write(modifiedJSON)
                            file.close()
        except Exception as e:
            raise Exception("translate_db_root_path_to_tmp_dir() Error: " + str(e))

    def create_tmp_config_files(self, testingArgs: TestingArgs) -> TestingArgs:
        """
        Creates temporary copies of VDMS config files in the temporary directory.

        This method checks if the `config_files_for_vdms` attribute in the
        `testingArgs` object is set and valid. It then creates temporary copies
        of these config files in the directory specified by
        `testingArgs.tmp_tests_dir`.

        Parameters:
        - testing_args (TestingArgs): An instance of the `TestingArgs` class
                which contains various arguments and configurations for testing.
                This object should have attributes `config_files_for_vdms` and
                `tmp_tests_dir`.

        Returns:
        - TestingArgs: The updated `testingArgs` object with the
            `tmp_config_files_for_vdms` attribute set to the list of temporary
            config file paths.

        Raises:
            Exception: If an error occurs while creating the temporary config
            files or if any of the config files do not exist or are not
            accessible.

        Notes:
            - The method first checks if `tmp_tests_dir` is a valid directory.
            - It then checks if `config_files_for_vdms` is set and valid.
            - For each config file, it creates a temporary copy in the
            `tmp_tests_dir` directory.
            - The list of temporary config file paths is stored in
            `tmp_config_files_for_vdms`.
        """
        try:
            tmpConfigFileList = []
            if not os.path.exists(testingArgs.tmp_tests_dir):
                raise Exception(
                    f"tmp_tests_dir: {testingArgs.tmp_tests_dir} is not a valid dir"
                )

            if (
                hasattr(testingArgs, "config_files_for_vdms")
                and testingArgs.config_files_for_vdms is not None
            ):
                for configFile in testingArgs.config_files_for_vdms:
                    if not os.path.exists(configFile):
                        raise Exception(
                            f"Config file: {configFile} is not a valid path"
                        )

                    tmpConfigFile = os.path.join(
                        testingArgs.tmp_tests_dir, os.path.basename(configFile)
                    )

                    shutil.copy2(configFile, tmpConfigFile)
                    tmpConfigFileList.append(tmpConfigFile)
                testingArgs.tmp_config_files_for_vdms = tmpConfigFileList
            else:
                raise Exception("config_files_for_vdms value is invalid")

        except Exception as e:
            raise Exception("create_tmp_config_files() Error: " + str(e))

        return testingArgs

    def create_dirs(self, dirs: list):
        """
        Creates directories specified in the list, removing them first if they
        already exist.

        This method iterates over a list of directory paths. For each directory,
        it checks if the directory exists. If it does, the directory is removed.
        Then, a new directory is created at the specified path.

        Parameters:
        - dirs (list): A list of directory paths to be created.

        Raises:
            Exception: If an error occurs while creating the directories.

        Notes:
            - If a directory already exists, it is removed before creating a new
            one.
            - The method uses `shutil.rmtree` to remove existing directories and
            `os.makedirs` to create new directories.
        """
        try:
            for dir in dirs:
                if os.path.exists(dir):
                    shutil.rmtree(dir, ignore_errors=False)
                os.makedirs(dir)
        except Exception as e:
            raise Exception("create_dirs() Error: " + str(e))

    def setup(self, testingArgs: TestingArgs) -> TestingArgs:
        """
        Sets up the testing environment by creating necessary directories and
        configuring VDMS config files.

        This method performs the following steps:
        1. Creates the temporary directory specified in `testingArgs.tmp_tests_dir`.
        2. Creates temporary copies of the VDMS config files and updates the
        `db_root_path` to use a local directory inside the temporary directory.
        3. Translates the `db_root_path` in the VDMS config files to point to a
        directory inside the temporary directory.

        Parameters:
        - testingArgs (TestingArgs): An instance of the `TestingArgs` class
                which contains various arguments and configurations for testing.

        Returns:
        - TestingArgs: The updated `testingArgs` object with the setup
            completed.

        Notes:
            - The method first checks if `tmp_tests_dir` is specified and creates
            the directory if it is.
            - It then creates temporary copies of the VDMS config files and
            updates the `db_root_path` to point to a directory inside the
            temporary directory.
            - The method finally translates the `db_root_path` in the VDMS config
            files to point to a directory inside the temporary directory.
        """
        # Create the tmp dir
        if testingArgs.tmp_tests_dir is not None:
            self.create_dirs([testingArgs.tmp_tests_dir])

        # Create a copy of the config files and update the db_root_path
        # to use a local dir inside of the tmp directory
        testingArgs = self.create_tmp_config_files(testingArgs)

        # The VDMS config files contain an attribute called db_root_path
        # which creates a directory named by using the value of that attribute
        # in order to create the dir inside of the temporary dir then
        # a new tmp config file is duplicated and modified to use
        # a tmp db_root_path which points to a dir inside of the tmp dir
        self.translate_db_root_path_to_tmp_dir(testingArgs)

        return testingArgs

    def execute_test(self, testingArgs: TestingArgs):
        """
        Executes the specified type of test after setting up the testing
        environment.

        This method performs the following steps:
        1. Sets up the testing environment by calling the `setup` method.
        2. Executes the appropriate type of test based on the `type_of_test`
        attribute in the `testingArgs` object.

        Parameters:
        - testingArgs (TestingArgs): An instance of the `TestingArgs` class
                which contains various arguments and configurations for testing.

        Raises:
            Exception: If an error occurs while setting up or executing the test.
        """
        if DEBUG_MODE:
            print("TestingParser::execute_test() was called")

        try:
            testingArgs = self.setup(testingArgs)

            if testingArgs.type_of_test == "pt":
                tests = NonRemotePythonTest()
                tests.run(testingArgs)
            elif testingArgs.type_of_test == "rp":
                tests = RemotePythonTest()
                tests.run(testingArgs)
            elif testingArgs.type_of_test == "ut":
                tests = NonRemoteTest()
                tests.run(testingArgs)
            elif testingArgs.type_of_test == "neo":
                tests = Neo4jTest()
                tests.run(testingArgs)
            elif testingArgs.type_of_test == "ru":
                tests = RemoteTest()
                tests.run(testingArgs)
        except Exception as e:
            raise Exception("execute_test() Error: " + str(e))

    def merge_testing_args(
        self, testingArgsFromJSONFile: TestingArgs, testingArgsFromCmdLine: TestingArgs
    ) -> TestingArgs:
        """
        Merges two TestingArgs objects, giving priority to the command line
        arguments.

        If an argument is set in both the JSON file and the command line, the
        command line value is used.

        Parameters:
        - testingArgsFromJSONFile (TestingArgs): TestingArgs object with
                arguments from JSON file.
        - testingArgsFromCmdLine (TestingArgs): TestingArgs object with
                arguments from command line.

        Returns:
        - TestingArgs: A new TestingArgs object with merged arguments.

        Raises:
            Exception: If an error occurs while merging the arguments.

        Notes:
            - If `testingArgsFromJSONFile` is None, the method returns
            `testingArgsFromCmdLine`.
            - The method creates a deep copy of the JSON file arguments to avoid
            modifying the original object.
            - It then iterates over the attributes of `testingArgsFromCmdLine`
            and sets them in the merged object if they are not None.
        """
        if DEBUG_MODE:
            print("merge_testing_args() was called")

        try:
            if testingArgsFromJSONFile is None:
                return testingArgsFromCmdLine

            # Create a deep copy of the JSON file arguments to avoid modifying the original object
            merged_testing_args = copy.deepcopy(testingArgsFromJSONFile)

            if testingArgsFromCmdLine is not None:
                for attribute, value in vars(testingArgsFromCmdLine).items():
                    if value is not None:
                        setattr(merged_testing_args, attribute, value)

            return merged_testing_args
        except Exception as e:
            raise Exception("merge_testing_args() Error: " + str(e))

    def convert_to_absolute_paths(self, testingArgs: TestingArgs) -> TestingArgs:
        """
        Converts relative paths in the TestingArgs object to absolute paths.

        This method checks various attributes in the `testingArgs` object and
        converts them to absolute paths if they are not already absolute.

        Parameters:
        - testingArgs (TestingArgs): An instance of the `TestingArgs` class
                which contains various arguments and configurations for testing.

        Returns:
        - TestingArgs: The updated `testingArgs` object with absolute paths.

        Notes:
            - The method converts the following attributes to absolute paths if
            they are not already absolute:
                - `tmp_tests_dir`
                - `config_files_for_vdms`
                - `tmp_config_files_for_vdms`
                - `minio_app_path`
                - `vdms_app_path`
                - `googletest_path`
        """
        if DEBUG_MODE:
            print("convert_to_absolute_paths() was called")

        if (
            hasattr(testingArgs, "tmp_tests_dir")
            and testingArgs.tmp_tests_dir is not None
            and not os.path.isabs(testingArgs.tmp_tests_dir)
        ):
            testingArgs.tmp_tests_dir = os.path.abspath(testingArgs.tmp_tests_dir)

        if (
            hasattr(testingArgs, "config_files_for_vdms")
            and testingArgs.config_files_for_vdms is not None
        ):
            configFiles = []
            for index in range(len(testingArgs.config_files_for_vdms)):
                config_file = testingArgs.config_files_for_vdms[index]
                if not os.path.isabs(config_file):
                    config_file = os.path.abspath(config_file)

                configFiles.append(config_file)
            testingArgs.config_files_for_vdms = configFiles

        if (
            hasattr(testingArgs, "tmp_config_files_for_vdms")
            and testingArgs.tmp_config_files_for_vdms is not None
            and isinstance(testingArgs.tmp_config_files_for_vdms, list)
        ):
            tmpConfigFiles = []
            for index in range(len(testingArgs.tmp_config_files_for_vdms)):
                config_file = testingArgs.tmp_config_files_for_vdms[index]
                if not os.path.isabs(config_file):
                    config_file = os.path.abspath(config_file)
                tmpConfigFiles.append(config_file)
            testingArgs.tmp_config_files_for_vdms = tmpConfigFiles

        if (
            hasattr(testingArgs, "minio_app_path")
            and testingArgs.minio_app_path is not None
            and not os.path.isabs(testingArgs.minio_app_path)
        ):
            testingArgs.minio_app_path = os.path.abspath(testingArgs.minio_app_path)

        if (
            hasattr(testingArgs, "vdms_app_path")
            and testingArgs.vdms_app_path is not None
            and not os.path.isabs(testingArgs.vdms_app_path)
        ):
            testingArgs.vdms_app_path = os.path.abspath(testingArgs.vdms_app_path)

        if (
            hasattr(testingArgs, "googletest_path")
            and testingArgs.googletest_path is not None
            and not os.path.isabs(testingArgs.googletest_path)
        ):
            testingArgs.googletest_path = os.path.abspath(testingArgs.googletest_path)

        return testingArgs

    def print_help(self, parser: argparse.ArgumentParser):
        """
        Prints the help message for the argument parser.

        This method calls the `print_help` method of the provided `argparse`
        parser to display the help message.

        Parameters:
        - parser (argparse.ArgumentParser): The argument parser for which the
                help message should be printed.
        """
        parser.print_help()

    def print_error(self, errorMessage: str, parser: argparse.ArgumentParser):
        """
        Prints an error message and exits the program.

        This method calls the `error` method of the provided `argparse` parser to
        display the error message and exit the program.

        Parameters:
        - errorMessage (str): The error message to be displayed.
        - parser (argparse.ArgumentParser): The argument parser used to display
                the error message.
        """
        parser.error(errorMessage)


def main():
    """
    Main function to parse arguments, set up the testing environment, and
    execute tests.

    This function performs the following steps:
    1. Parses command line arguments.
    2. Reads configuration from a JSON file if specified.
    3. Merges arguments from the JSON file and command line, giving priority
       to command line arguments.
    4. Converts relative paths to absolute paths.
    5. Validates the arguments.
    6. Fills in default arguments.
    7. Executes the specified tests.
    8. Cleans up processes, log files, and temporary files if the `run`
       flag is set to True.

    Raises:
        Exception: If an error occurs during any of the steps, the error is
        printed and the program exits with a status code of 1.

    Notes:
        - If the `run` flag is set to False then it skips the call to the
            googletest or unittest binaries, this way you can start all the
            other binaries needed for debugging purposes and use your debugger
            manually to call to googletest (C++) or unittest (Python) binaries.
    """
    if DEBUG_MODE:
        print("main() was called")

    try:
        testingParser = TestingParser()
        args = testingParser.parse_arguments()

        testingArgsFromJSONFile = None
        if args.json is not None:
            testingArgsFromJSONFile = testingParser.read_json_config_file(
                args.json, testingParser.get_parser()
            )
            print(f"Note: -j/--json argument was provided: {args.json}")
            print(
                "\tIf there are other arguments in the command line then they will have a higher priority than the ones found in the JSON file"
            )

        testingArgsFromCmdLine = testingParser.convert_namespace_to_testing_args(
            args, testingParser.get_parser()
        )

        # The parser can read arguments from a JSON file and the command line
        # If there is a conflict among arguments which are set in both JSON file
        # and command line then the priority will be given to the command line
        # arguments
        testingArgs = testingParser.merge_testing_args(
            testingArgsFromJSONFile, testingArgsFromCmdLine
        )

        # Convert any relative paths to absolute paths
        testingArgs = testingParser.convert_to_absolute_paths(testingArgs)

        # Once the data was parsed then it must be validated
        testingParser.validate_arguments(testingArgs, testingParser.get_parser())
        testingArgs = testingParser.fill_default_arguments(testingArgs)

        if DEBUG_MODE:
            print(testingArgs)

        testingParser.execute_test(testingArgs)

        # In case the user doesn't want to run the tests then it assumes
        # that this script is going to be used for debugging purposes
        # therefore it is neither going to kill the running processes nor
        # it is going to close the log files nor deleting the temporary files
        if testingArgs.run:
            if DEBUG_MODE:
                print("TestingParser::main() Calling to clean-up functions")
            kill_processes_by_object()
            close_log_files()
            cleanup()
        else:
            print("Warning: --run flag is set to False")
        exit(0)
    except Exception as e:
        kill_processes_by_object()
        close_log_files()
        cleanup()
        testingParser.print_error("main() Error: " + str(e), testingParser.get_parser())
        exit(1)


if __name__ == "__main__":
    # TODO: Delete this debugging message
    if DEBUG_MODE:
        print("__main__ was called")
    main()
