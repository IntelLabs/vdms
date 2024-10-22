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

import os
import argparse
import subprocess
from io import StringIO
import json
import signal
import unittest
from json.decoder import JSONDecodeError
from unittest.mock import patch, mock_open, MagicMock, Mock, call

from run_all_tests import (
    TestingParser,
    RemotePythonTest,
    RemoteTest,
    NonRemotePythonTest,
    NonRemoteTest,
    Neo4jTest,
    TestingArgs,
    AbstractTest,
)
from run_all_tests import (
    TYPE_OF_TESTS_AVAILABLE,
    DEFAULT_REMOTE_PYTHON_TEST_FILTER,
    DEFAULT_REMOTE_PYTHON_CONFIG_FILES,
    DEFAULT_REMOTE_UNIT_TEST_FILTER,
    DEFAULT_NON_REMOTE_PYTHON_TEST_FILTER,
    DEFAULT_NON_REMOTE_PYTHON_CONFIG_FILES,
    DEFAULT_NON_REMOTE_UNIT_TEST_FILTER,
    DEFAULT_NON_REMOTE_UNIT_TEST_CONFIG_FILES,
    DEFAULT_VDMS_APP_PATH,
    DEFAULT_MINIO_PATH,
    DEFAULT_MINIO_TMP_DIR,
    DEFAULT_MINIO_ALIAS_NAME,
    DEFAULT_MINIO_PORT,
    DEFAULT_MINIO_CONSOLE_PORT,
    DEFAULT_GOOGLETEST_PATH,
    DEFAULT_NEO_TEST_PORT,
    DEFAULT_NEO_TEST_ENDPOINT,
    STOP_ON_FAILURE_FLAG,
    DEFAULT_DIR_REPO,
    DEFAULT_NEO4J_E2E_TEST_FILTER,
    DEFAULT_NEO4J_OPSIO_TEST_FILTER,
    DEFAULT_NEO4J_E2E_CONFIG_FILES,
    DEFAULT_TMP_DIR,
    DEFAULT_TESTS_STDERR_FILENAME,
    DEFAULT_TESTS_STDOUT_FILENAME,
    NEO4J_OPS_IO_TEST_TYPE,
    NEO4J_E2E_TEST_TYPE,
    NEO4J_BACKEND_TEST_TYPE,
    main,
)

import run_all_tests
import sys
import io


#### Concrete class inherited from AbstractTest
#### This class is used for testing purposes
class ConcreteClass(AbstractTest):
    def run(self, testingArgs: TestingArgs):
        return

    def validate_arguments(
        self, testingArgs: TestingArgs, parser: argparse.ArgumentParser
    ):
        return

    def fill_default_arguments(self, testingArgs: TestingArgs) -> TestingArgs:
        return TestingArgs()


# Mock process object
class MockProcess:
    def __init__(self, pid):
        self.pid = pid
        self.kill = MagicMock()


class TestKillProcessesByObject(unittest.TestCase):

    def setUp(self):
        # Set up for each test
        self.original_processList = run_all_tests.processList
        run_all_tests.processList = [
            MockProcess(123),
            MockProcess(456),
            MockProcess(789),
        ]
        self.original_DEBUG_MODE = run_all_tests.DEBUG_MODE
        run_all_tests.DEBUG_MODE = True

    def tearDown(self):
        # Clean up after each test
        run_all_tests.processList = self.original_processList
        run_all_tests.DEBUG_MODE = self.original_DEBUG_MODE

    @patch("os.system")
    @patch("run_all_tests.print")
    def test_kill_processes_by_object(self, mock_print, mock_system):
        run_all_tests.kill_processes_by_object()

        # Check if the correct print statements were made
        expected_print_calls = [
            unittest.mock.call("Killing 3 processes"),
            unittest.mock.call("Killing pid: 789"),
            unittest.mock.call("Killing pid: 456"),
            unittest.mock.call("Killing pid: 123"),
        ]

        # Check if debug messages were printed
        mock_print.assert_has_calls(expected_print_calls, any_order=False)

        # Check if pidList is cleared
        self.assertEqual(run_all_tests.processList, [])

    def test_kill_processes_by_object_exception(self):
        # Make the kill method of the first process object raise an exception
        run_all_tests.processList[0].kill.side_effect = Exception("Test Exception")

        # Capture the output of print statements
        captured_output = io.StringIO()
        sys.stdout = captured_output

        run_all_tests.kill_processes_by_object()

        # Reset stdout
        sys.stdout = sys.__stdout__

        # Check that the exception message was printed
        output = captured_output.getvalue()
        self.assertIn("Warning: kill_processes_by_object(): Test Exception", output)

        # Check that the processList is not cleared due to the exception
        self.assertNotEqual(run_all_tests.processList, [])


#### Test suite for the close_log_files function ####
class TestCloseLogFiles(unittest.TestCase):

    def setUp(self):
        # Set up mock file descriptors and global variables before each test
        self.original_fdList = run_all_tests.fdList
        self.mock_file_1 = MagicMock()
        self.mock_file_1.closed = False
        self.mock_file_1.name = "file1.log"
        self.mock_file_2 = MagicMock()
        self.mock_file_2.closed = False
        self.mock_file_2.name = "file2.log"
        run_all_tests.fdList = [self.mock_file_1, self.mock_file_2]
        self.original_DEBUG_MODE = run_all_tests.DEBUG_MODE
        run_all_tests.DEBUG_MODE = False

    def tearDown(self):
        # Reset global variables after each test
        run_all_tests.fdList = self.original_fdList
        run_all_tests.DEBUG_MODE = self.original_DEBUG_MODE

    def test_close_log_files(self):
        # Test that all file descriptors are closed and fdList is cleared
        run_all_tests.close_log_files()
        # Assert each file descriptor's close method was called
        self.mock_file_1.close.assert_called_once()
        self.mock_file_2.close.assert_called_once()
        # Assert fdList is empty after closing files
        self.assertEqual(run_all_tests.fdList, [])

    @patch("run_all_tests.print")
    def test_close_log_files_with_debug(self, mock_print):
        # Test that debug messages are printed and files are closed when DEBUG_MODE is True
        run_all_tests.DEBUG_MODE = True
        run_all_tests.close_log_files()
        # Assert print was called with the correct messages for each file
        mock_print.assert_any_call(f"Closing: {self.mock_file_1.name}")
        mock_print.assert_any_call(f"Closing: {self.mock_file_2.name}")
        # Assert fdList is empty after closing files
        self.assertEqual(run_all_tests.fdList, [])

    def test_close_log_files_exception(self):
        # Test behavior when an exception occurs during file closing
        # Simulate an exception for the first file descriptor
        self.mock_file_1.close.side_effect = Exception("Test Exception")
        with patch("run_all_tests.print") as mock_print:
            run_all_tests.close_log_files()
            # Assert a warning message was printed due to the exception
            mock_print.assert_called_with("Warning: close_log_files(): Test Exception")
        # Assert fdList is not cleared due to the exception
        self.assertEqual(run_all_tests.fdList, [self.mock_file_1, self.mock_file_2])


#### Test suite the cleanup() function ####
class TestCleanup(unittest.TestCase):

    @patch("run_all_tests.print")
    @patch("run_all_tests.os.path.exists", return_value=True)
    @patch("run_all_tests.shutil.rmtree")
    def test_cleanup_remove_dir(self, mock_rmtree, mock_exists, mock_print):
        # Test cleanup when the directory should be removed
        run_all_tests.DEBUG_MODE = False
        run_all_tests.global_keep_tmp_tests_dir = False
        run_all_tests.global_tmp_tests_dir = "/path/to/temp/dir"
        run_all_tests.cleanup()
        # Assert rmtree was called with the correct path
        mock_rmtree.assert_called_once_with("/path/to/temp/dir", ignore_errors=False)

    @patch("run_all_tests.os.path.exists", return_value=True)
    @patch("run_all_tests.shutil.rmtree")
    def test_cleanup_keep_dir(self, mock_rmtree, mock_exists):
        # Test cleanup when the directory should not be removed
        run_all_tests.global_keep_tmp_tests_dir = True
        run_all_tests.global_tmp_tests_dir = "/path/to/temp/dir"
        run_all_tests.cleanup()
        # Assert rmtree was not called
        mock_rmtree.assert_not_called()

    @patch("run_all_tests.os.path.exists", return_value=False)
    @patch("run_all_tests.shutil.rmtree")
    def test_cleanup_no_dir(self, mock_rmtree, mock_exists):
        # Test cleanup when the directory does not exist
        run_all_tests.global_keep_tmp_tests_dir = False
        run_all_tests.global_tmp_tests_dir = "/path/to/nonexistent/dir"
        run_all_tests.cleanup()
        # Assert rmtree was not called
        mock_rmtree.assert_not_called()

    @patch("run_all_tests.os.path.exists", return_value=True)
    @patch("run_all_tests.shutil.rmtree", side_effect=Exception("Mocked exception"))
    def test_cleanup_exception(self, mock_rmtree, mock_exists):
        # Test cleanup when rmtree raises an exception
        run_all_tests.global_keep_tmp_tests_dir = False
        run_all_tests.global_tmp_tests_dir = "/path/to/temp/dir"
        with self.assertRaises(Exception) as context:
            run_all_tests.cleanup()
        # Assert the exception was raised and contains the correct message
        self.assertTrue("Mocked exception" in str(context.exception))

    @patch("run_all_tests.print")
    @patch("run_all_tests.os.path.exists", return_value=True)
    @patch("run_all_tests.shutil.rmtree")
    def test_cleanup_debug_mode_true(self, mock_rmtree, mock_exists, mock_print):
        # Set up the global variables for the test
        run_all_tests.DEBUG_MODE = True
        run_all_tests.global_keep_tmp_tests_dir = False
        run_all_tests.global_tmp_tests_dir = "/path/to/temp/dir"

        # Call the cleanup function, which is expected to print debug messages and delete the directory
        run_all_tests.cleanup()

        # Define the expected calls to the print function
        expected_print_calls = [
            call("Cleaning up"),
            call("Deleting the directory:", "/path/to/temp/dir"),
        ]

        # Check if the expected calls were made
        mock_print.assert_has_calls(expected_print_calls, any_order=False)

        # Check if the directory was attempted to be deleted
        mock_rmtree.assert_called_once_with("/path/to/temp/dir", ignore_errors=False)

        # Reset the global variables if necessary
        run_all_tests.DEBUG_MODE = False
        run_all_tests.global_keep_tmp_tests_dir = True
        run_all_tests.global_tmp_tests_dir = None


#### Test suite for the signal_handler function ####
class TestSignalHandler(unittest.TestCase):

    @patch("run_all_tests.print")
    def test_signal_handler_sigterm(self, mock_print):
        # Test that the handler exits with code 0 on SIGTERM and calls the necessary functions
        run_all_tests.DEBUG_MODE = True
        run_all_tests.signal_handler(signal.SIGTERM, None)
        # Assert that the print function was called with the correct message
        mock_print.assert_called_with(f"Caught signal {str(signal.SIGTERM)}. Exiting.")

    @patch("run_all_tests.print")
    @patch("run_all_tests.close_log_files")
    @patch("run_all_tests.kill_processes_by_object")
    @patch("run_all_tests.exit")
    def test_signal_handler_sigabrt(
        self, mock_exit, mock_killProcesses, mock_closeLogFiles, mock_print
    ):
        # Test that the handler exits with code 1 on SIGABRT and calls the necessary functions
        run_all_tests.DEBUG_MODE = True
        run_all_tests.signal_handler(signal.SIGABRT, None)
        # Assert that the print function was called with the correct message
        mock_print.assert_called_with(
            f"Caught {str(signal.SIGABRT)}. Exiting gracefully."
        )
        # Assert that close_log_files and kill_processes were called
        mock_closeLogFiles.assert_called_once()
        mock_killProcesses.assert_called_once()
        # Assert that the exit function was called with status code 1
        mock_exit.assert_called_with(1)

    @patch("run_all_tests.print")
    @patch("run_all_tests.close_log_files")
    @patch("run_all_tests.kill_processes_by_object")
    @patch("run_all_tests.exit")
    def test_signal_handler_sigint(
        self, mock_exit, mock_killProcesses, mock_closeLogFiles, mock_print
    ):
        # Test that the handler exits with code 1 on SIGINT and calls the necessary functions
        run_all_tests.DEBUG_MODE = True
        run_all_tests.signal_handler(signal.SIGINT, None)
        # Assert that the print function was called with the correct message
        mock_print.assert_called_with(
            f"Caught {str(signal.SIGINT)}. Exiting gracefully."
        )
        # Assert that close_log_files and kill_processes were called
        mock_closeLogFiles.assert_called_once()
        mock_killProcesses.assert_called_once()
        # Assert that the exit function was called with status code 1
        mock_exit.assert_called_with(1)

    @patch("run_all_tests.print")
    @patch("run_all_tests.close_log_files")
    @patch("run_all_tests.kill_processes_by_object")
    @patch("run_all_tests.exit")
    def test_signal_handler_sigsegv(
        self, mock_exit, mock_killProcesses, mock_closeLogFiles, mock_print
    ):
        # Test that the handler exits with code 0 on SIGSEGV and calls the necessary functions
        run_all_tests.DEBUG_MODE = True
        run_all_tests.signal_handler(signal.SIGSEGV, None)
        # Assert that the print function was called with the correct message
        mock_print.assert_called_with(
            f"Caught {str(signal.SIGSEGV)}. Exiting gracefully."
        )
        # Assert that close_log_files and kill_processes were called
        mock_closeLogFiles.assert_called_once()
        mock_killProcesses.assert_called_once()
        # Assert that the exit function was called with status code 1
        mock_exit.assert_called_with(1)

    @patch("run_all_tests.print")
    @patch("run_all_tests.close_log_files")
    @patch("run_all_tests.kill_processes_by_object")
    @patch("run_all_tests.exit")
    def test_signal_handler_no_debug(
        self, mock_exit, mock_killProcesses, mock_closeLogFiles, mock_print
    ):
        # Test that the handler calls the necessary functions without printing messages when DEBUG_MODE is False
        run_all_tests.DEBUG_MODE = False
        run_all_tests.signal_handler(signal.SIGABRT, None)
        # Assert that the print function was not called
        mock_print.assert_not_called()
        # Assert that close_log_files and kill_processes were called
        mock_closeLogFiles.assert_called_once()
        mock_killProcesses.assert_called_once()
        # Assert that the exit function was called with status code 1
        mock_exit.assert_called_with(1)

    @patch("run_all_tests.print")
    @patch("run_all_tests.close_log_files")
    @patch("run_all_tests.kill_processes_by_object")
    @patch("run_all_tests.exit")
    def test_signal_handler_unhandled_signal(
        self, mock_exit, mock_killProcesses, mock_closeLogFiles, mock_print
    ):
        # Test that the handler does not call any functions for unhandled signals
        run_all_tests.DEBUG_MODE = True
        unhandled_signal = (
            signal.SIGUSR1
        )  # Example of a signal that is not handled by the function
        run_all_tests.signal_handler(unhandled_signal, None)
        # Assert that the print function was called with the correct message
        mock_print.assert_called_with(
            f"Caught signal {str(unhandled_signal)}. Exiting."
        )
        # Assert that close_log_files and kill_processes were not called
        mock_closeLogFiles.assert_not_called()
        mock_killProcesses.assert_not_called()
        # Assert that the exit function was not called
        mock_exit.assert_not_called()


#### Testing the TestingArgs class ####
class TestTestingArgs(unittest.TestCase):
    def fill_attributes_for_testing(self) -> TestingArgs:
        args = TestingArgs()
        args.test_name = "Unit Test"
        args.minio_username = "minio_user"
        args.minio_password = "minio_pass"
        args.type_of_test = "integration"
        args.tmp_tests_dir = "/tmp/tests"
        args.config_files_for_vdms = ["config1", "config2"]
        args.tmp_config_files_for_vdms = ["tmp_config1", "tmp_config2"]
        args.minio_app_path = "/minio/app"
        args.minio_tmp_dir_name = "minio_tmp"
        args.minio_port = 9000
        args.minio_alias_name = "minio_alias"
        args.stop_tests_on_failure = True
        args.vdms_app_path = "/vdms/app"
        args.googletest_path = "/google/test"
        args.keep_tmp_tests_dir = True
        args.stderr_filename = "stderr.log"
        args.stdout_filename = "stdout.log"
        args.minio_console_port = 9001
        args.neo4j_port = 7474
        args.neo4j_password = "neo4j_pass"
        args.neo4j_username = "neo4j_user"
        args.run = True
        return args

    def test_initialization_with_none(self):
        args = TestingArgs()
        # Test that all attributes are initialized to None
        for attr in vars(args):
            with self.subTest(attr=attr):
                self.assertIsNone(getattr(args, attr))

    def test_setting_attributes(self):
        # Create an instance with specific values
        args = self.fill_attributes_for_testing()

        # Test that all attributes are set correctly
        self.assertEqual(args.test_name, "Unit Test")
        self.assertEqual(args.minio_username, "minio_user")
        self.assertEqual(args.minio_password, "minio_pass")
        self.assertEqual(args.type_of_test, "integration")
        self.assertEqual(args.tmp_tests_dir, "/tmp/tests")
        self.assertEqual(args.config_files_for_vdms, ["config1", "config2"])
        self.assertEqual(args.tmp_config_files_for_vdms, ["tmp_config1", "tmp_config2"])
        self.assertEqual(args.minio_app_path, "/minio/app")
        self.assertEqual(args.minio_tmp_dir_name, "minio_tmp")
        self.assertEqual(args.minio_port, 9000)
        self.assertEqual(args.minio_alias_name, "minio_alias")
        self.assertEqual(args.stop_tests_on_failure, True)
        self.assertEqual(args.vdms_app_path, "/vdms/app")
        self.assertEqual(args.googletest_path, "/google/test")
        self.assertEqual(args.keep_tmp_tests_dir, True)
        self.assertEqual(args.stderr_filename, "stderr.log")
        self.assertEqual(args.stdout_filename, "stdout.log")
        self.assertEqual(args.minio_console_port, 9001)
        self.assertEqual(args.neo4j_port, 7474)
        self.assertEqual(args.neo4j_password, "neo4j_pass")
        self.assertEqual(args.neo4j_username, "neo4j_user")
        self.assertEqual(args.run, True)

    def test_repr_representation(self):
        args = self.fill_attributes_for_testing()

        repr_representation = repr(args)
        self.assertIn("TestingArgs", repr_representation)
        self.assertIn("test_name='Unit Test'", repr_representation)
        self.assertIn("minio_username='minio_user'", repr_representation)
        self.assertIn("type_of_test='integration'", repr_representation)
        self.assertIn("tmp_tests_dir='/tmp/tests'", repr_representation)
        self.assertIn(
            "config_files_for_vdms=['config1', 'config2']", repr_representation
        )
        self.assertIn(
            "tmp_config_files_for_vdms=['tmp_config1', 'tmp_config2']",
            repr_representation,
        )
        self.assertIn("minio_app_path='/minio/app'", repr_representation)
        self.assertIn("minio_tmp_dir_name='minio_tmp'", repr_representation)
        self.assertIn("minio_port=9000", repr_representation)
        self.assertIn("minio_alias_name='minio_alias'", repr_representation)
        self.assertIn("stop_tests_on_failure=True", repr_representation)
        self.assertIn("vdms_app_path='/vdms/app'", repr_representation)
        self.assertIn("googletest_path='/google/test'", repr_representation)
        self.assertIn("keep_tmp_tests_dir=True", repr_representation)
        self.assertIn("stderr_filename='stderr.log'", repr_representation)
        self.assertIn("stdout_filename='stdout.log'", repr_representation)
        self.assertIn("minio_console_port=9001", repr_representation)
        self.assertIn("neo4j_port=7474", repr_representation)
        self.assertIn("neo4j_username='neo4j_user'", repr_representation)
        self.assertIn("run=True", repr_representation)

        # Check that sensitive information is not in the repr
        self.assertNotIn("minio_password", repr_representation)
        self.assertNotIn("neo4j_password", repr_representation)

    def test_str_representation(self):
        args = self.fill_attributes_for_testing()

        # Get the string representation using __str__
        str_representation = str(args)

        # Verify that the string representation contains specific attribute values
        self.assertIn("test_name='Unit Test'", str_representation)
        self.assertIn("minio_username='minio_user'", str_representation)
        self.assertIn("type_of_test='integration'", str_representation)
        self.assertIn("tmp_tests_dir='/tmp/tests'", str_representation)
        self.assertIn(
            "config_files_for_vdms=['config1', 'config2']", str_representation
        )
        self.assertIn(
            "tmp_config_files_for_vdms=['tmp_config1', 'tmp_config2']",
            str_representation,
        )
        self.assertIn("minio_app_path='/minio/app'", str_representation)
        self.assertIn("minio_tmp_dir_name='minio_tmp'", str_representation)
        self.assertIn("minio_port=9000", str_representation)
        self.assertIn("minio_alias_name='minio_alias'", str_representation)
        self.assertIn("stop_tests_on_failure=True", str_representation)
        self.assertIn("vdms_app_path='/vdms/app'", str_representation)
        self.assertIn("googletest_path='/google/test'", str_representation)
        self.assertIn("keep_tmp_tests_dir=True", str_representation)
        self.assertIn("stderr_filename='stderr.log'", str_representation)
        self.assertIn("stdout_filename='stdout.log'", str_representation)
        self.assertIn("minio_console_port=9001", str_representation)
        self.assertIn("neo4j_port=7474", str_representation)
        self.assertIn("neo4j_username='neo4j_user'", str_representation)
        self.assertIn("run=True", str_representation)

        # Verify that sensitive information is not in the string representation
        self.assertNotIn("minio_password", str_representation)
        self.assertNotIn("neo4j_password", str_representation)


#### Tests for the AbstractTest class ####
class TestAbstractTest(unittest.TestCase):

    def setUp(self):
        # Set up for each test
        self.original_DEBUG_MODE = run_all_tests.DEBUG_MODE
        run_all_tests.DEBUG_MODE = True

    def tearDown(self):
        # Clean up after each test
        run_all_tests.DEBUG_MODE = self.original_DEBUG_MODE

    def test_abstract_test_cannot_be_instantiated(self):
        # Attempt to instantiate AbstractTest and expect a TypeError
        with self.assertRaises(TypeError):
            abstract_test = AbstractTest()

    def test_abstract_methods_are_defined(self):
        # Create a temporary concrete subclass within the test
        class TemporaryConcreteTest(AbstractTest):
            def run(self, testingArgs: TestingArgs):
                super().run(testingArgs)

            def validate_arguments(
                self, testingArgs: TestingArgs, parser: argparse.ArgumentParser
            ):
                super().validate_arguments(testingArgs, parser)

            def fill_default_arguments(self, testingArgs: TestingArgs) -> TestingArgs:
                return super().fill_default_arguments(testingArgs)

        # Instantiate the temporary concrete subclass
        concrete_test = TemporaryConcreteTest()

        # Test that calling the abstract methods does not raise a NotImplementedError
        try:
            args = TestingArgs()
            parser = argparse.ArgumentParser()
            concrete_test.run(args)
            concrete_test.validate_arguments(args, parser)
            result_args = concrete_test.fill_default_arguments(args)
            self.assertIsInstance(result_args, TestingArgs)
        except NotImplementedError as e:
            self.fail(f"Abstract method raised NotImplementedError: {e}")

    #### Tests for get_valid_test_name() ####
    @patch("run_all_tests.print")
    def test_get_valid_test_name_no_flag(self, mock_print):
        args = TestingArgs()
        default_filter = "default-test-filter"
        instance = ConcreteClass()
        run_all_tests.DEBUG_MODE = True

        result = instance.get_valid_test_name(args, default_filter)
        mock_print.assert_called_once_with(
            "Warning: No test name filter was specified, running the default tests only:",
            default_filter,
        )
        self.assertEqual(result, default_filter)

    @patch("run_all_tests.print")
    def test_get_valid_test_name_empty_string(self, mock_print):
        args = TestingArgs()
        args.test_name = ""
        default_filter = "default-test-filter"
        instance = ConcreteClass()

        result = instance.get_valid_test_name(args, default_filter)
        mock_print.assert_called_once_with(
            "Warning: No test name filter was specified, running the default tests only:",
            default_filter,
        )
        self.assertEqual(result, default_filter)

    @patch("run_all_tests.print")
    def test_get_valid_test_name_valid_name(self, mock_print):
        args = TestingArgs()
        args.test_name = "specific-test"

        default_filter = "default-test-filter"
        instance = ConcreteClass()

        result = instance.get_valid_test_name(args, default_filter)
        mock_print.assert_not_called()
        self.assertEqual(result, "specific-test")

    def test_get_valid_vdms_values_no_vdms_app_path(self):
        default_config_files = ["default_config_1", "default_config_2"]
        instance = ConcreteClass()

        args = TestingArgs()
        result = instance.get_valid_vdms_values(args, default_config_files)
        self.assertEqual(result, (DEFAULT_VDMS_APP_PATH, default_config_files))

    def test_get_valid_vdms_values_no_config_files(self):
        default_config_files = ["default_config_1", "default_config_2"]
        instance = ConcreteClass()

        args = TestingArgs()
        args.vdms_app_path = "/custom/path"

        result = instance.get_valid_vdms_values(args, default_config_files)
        self.assertEqual(result, ("/custom/path", default_config_files))

    def test_get_valid_vdms_values_with_all_values(self):
        default_config_files = ["default_config_1", "default_config_2"]
        instance = ConcreteClass()

        args = TestingArgs()
        args.vdms_app_path = "/custom/path"
        args.config_files_for_vdms = ["custom_config"]

        result = instance.get_valid_vdms_values(args, default_config_files)
        self.assertEqual(result, ("/custom/path", ["custom_config"]))

    #### Tests for get_valid_minio_values() ####
    @patch("run_all_tests.print")
    @patch.dict(os.environ, {}, clear=True)
    def test_get_valid_minio_values_default_path(self, mock_print):
        # Test that the default MinIO path is used when no path is provided
        instance = ConcreteClass()
        args = TestingArgs()
        args.tmp_tests_dir = "/tmp/tests"
        args.minio_console_port = 9001
        args.minio_port = 9000

        args.minio_app_path = None
        result = instance.get_valid_minio_values(args)
        mock_print.assert_called_with("Warning: Using default MinIO installation")
        self.assertEqual(result.minio_app_path, DEFAULT_MINIO_PATH)

    @patch("run_all_tests.print")
    @patch.dict(os.environ, {}, clear=True)
    def test_get_valid_minio_values_default_ports(self, mock_print):
        # Test that the default MinIO ports are used when no ports are provided
        instance = ConcreteClass()
        args = TestingArgs()
        args.tmp_tests_dir = "/tmp/tests"

        args.minio_port = None
        args.minio_console_port = None
        result = instance.get_valid_minio_values(args)
        expected_calls = [
            unittest.mock.call(
                "Warning: Using default MinIO port: {minio_port}".format(
                    minio_port=DEFAULT_MINIO_PORT
                )
            ),
            unittest.mock.call(
                "Warning: Using default MinIO console port: {minio_console_port}".format(
                    minio_console_port=DEFAULT_MINIO_CONSOLE_PORT
                )
            ),
        ]
        mock_print.assert_has_calls(expected_calls, any_order=True)
        self.assertEqual(result.minio_port, DEFAULT_MINIO_PORT)
        self.assertEqual(result.minio_console_port, DEFAULT_MINIO_CONSOLE_PORT)

    @patch("run_all_tests.print")
    @patch.dict(os.environ, {"AWS_API_PORT": "9001", "AWS_CONSOLE_PORT": "9002"})
    def test_get_valid_minio_values_env_ports(self, mock_print):
        # Test that environment variables are used for MinIO ports when provided
        instance = ConcreteClass()
        args = TestingArgs()
        args.tmp_tests_dir = "/tmp/tests"

        args.minio_port = None
        args.minio_console_port = None
        result = instance.get_valid_minio_values(args)
        expected_calls = [
            unittest.mock.call(
                "Warning: Using MinIO port: {minio_port} by using env var".format(
                    minio_port="9001"
                )
            ),
            unittest.mock.call(
                "Warning: Using MinIO console port: {minio_console_port} by using env var".format(
                    minio_console_port="9002"
                )
            ),
        ]
        mock_print.assert_has_calls(expected_calls, any_order=True)
        self.assertEqual(result.minio_port, "9001")
        self.assertEqual(result.minio_console_port, "9002")

    def test_get_valid_minio_values_tmp_dir(self):
        # Test that the MinIO temporary directory name is constructed correctly
        instance = ConcreteClass()
        args = TestingArgs()
        args.tmp_tests_dir = "/tmp/tests"

        result = instance.get_valid_minio_values(args)
        self.assertEqual(
            result.minio_tmp_dir_name, args.tmp_tests_dir + "/" + DEFAULT_MINIO_TMP_DIR
        )

    def test_get_valid_minio_values_alias_name(self):
        # Test that the default MinIO alias name is used when no alias name is provided
        instance = ConcreteClass()
        args = TestingArgs()
        args.tmp_tests_dir = "/tmp/tests"

        args.minio_alias_name = ""
        result = instance.get_valid_minio_values(args)
        self.assertEqual(result.minio_alias_name, DEFAULT_MINIO_ALIAS_NAME)

    #### Tests for get_valid_google_test_values() ####
    def test_get_valid_googletest_values_no_path(self):
        instance = ConcreteClass()
        args = TestingArgs()

        # Test that the default GoogleTest path is used when no path is provided
        args.googletest_path = None
        result = instance.get_valid_google_test_values(args)
        self.assertEqual(result.googletest_path, DEFAULT_GOOGLETEST_PATH)

    def test_get_valid_googletest_values_existing_path(self):
        instance = ConcreteClass()
        args = TestingArgs()

        # Test that an existing GoogleTest path is not overridden
        custom_path = "/custom/googletest/path"
        args.googletest_path = custom_path
        result = instance.get_valid_google_test_values(args)
        self.assertEqual(result.googletest_path, custom_path)

    #### Tests for get_valid_neo4j_values() ####
    @patch("run_all_tests.print")
    @patch.dict(os.environ, {}, clear=True)
    def test_get_valid_neo4j_values_default_values(self, mock_print):
        # Test that the default Neo4j port is used when no port is provided
        instance = ConcreteClass()
        args = TestingArgs()
        args.neo4j_port = None
        args.neo4j_endpoint = None
        result = instance.get_valid_neo4j_values(args)
        mock_print.assert_has_calls(
            [
                unittest.mock.call(
                    "Warning: Using default Neo4j port: {neo4j_port}".format(
                        neo4j_port=DEFAULT_NEO_TEST_PORT
                    )
                ),
                unittest.mock.call(
                    "Warning: Using default Neo4j endpoint: {neo4j_endpoint}".format(
                        neo4j_endpoint=DEFAULT_NEO_TEST_ENDPOINT
                    )
                ),
            ]
        )
        self.assertEqual(result.neo4j_port, DEFAULT_NEO_TEST_PORT)
        self.assertEqual(result.neo4j_endpoint, DEFAULT_NEO_TEST_ENDPOINT)

    @patch("run_all_tests.print")
    @patch.dict(os.environ, {"NEO_TEST_PORT": "7688"})
    @patch.dict(os.environ, {"NEO4J_ENDPOINT": "neo4j://neo4j:7688"})
    def test_get_valid_neo4j_values_env_port(self, mock_print):
        # Test that the Neo4j port from the environment variable is used when provided
        instance = ConcreteClass()
        args = TestingArgs()
        args.neo4j_port = None
        args.neo4j_endpoint = None
        result = instance.get_valid_neo4j_values(args)
        mock_print.assert_has_calls(
            [
                unittest.mock.call(
                    "Warning: Using Neo4j port: {neo4j_port} by using env var".format(
                        neo4j_port="7688"
                    )
                ),
                unittest.mock.call(
                    "Warning: Using Neo4j endpoint: {neo4j_endpoint} by using env var".format(
                        neo4j_endpoint="neo4j://neo4j:7688"
                    )
                ),
            ]
        )
        self.assertEqual(result.neo4j_endpoint, "neo4j://neo4j:7688")

    def test_get_valid_neo4j_values_existing_port(self):
        # Test that an existing Neo4j port is not overridden
        instance = ConcreteClass()
        args = TestingArgs()

        custom_port = 7474
        args.neo4j_port = custom_port
        result = instance.get_valid_neo4j_values(args)
        self.assertEqual(result.neo4j_port, custom_port)

    #### Tests for validate_google_test_path() ####
    @patch("os.path.exists", return_value=True)
    @patch.object(argparse.ArgumentParser, "error")
    def test_validate_google_test_path_exists(self, mock_error, mock_exists):
        # Test that no error is raised when the GoogleTest path exists
        instance = ConcreteClass()
        args = TestingArgs()
        parser = argparse.ArgumentParser()

        args.googletest_path = "/valid/path"
        instance.validate_google_test_path(args, parser)
        mock_exists.assert_called_with("/valid/path")
        mock_error.assert_not_called()

    @patch("os.path.exists", return_value=False)
    @patch.object(argparse.ArgumentParser, "error")
    def test_validate_google_test_path_not_exists(self, mock_error, mock_exists):
        # Test that an error is raised when the GoogleTest path does not exist
        instance = ConcreteClass()
        args = TestingArgs()
        parser = argparse.ArgumentParser()

        args.googletest_path = "/invalid/path"
        instance.validate_google_test_path(args, parser)
        mock_exists.assert_called_with("/invalid/path")
        mock_error.assert_called_once()

    #### Tests for validate_minio_values() ####
    @patch("os.path.exists", return_value=True)
    @patch.object(argparse.ArgumentParser, "error")
    def test_validate_minio_values_valid(self, mock_error, mock_exists):
        # Test that no error is raised when MinIO username and password are provided
        instance = ConcreteClass()
        args = TestingArgs()
        parser = argparse.ArgumentParser()

        args.minio_username = "user"
        args.minio_password = "pass"
        args.minio_app_path = "/valid/path"
        instance.validate_minio_values(args, parser)
        mock_exists.assert_called_with("/valid/path")
        mock_error.assert_not_called()

    @patch("os.path.exists", return_value=False)
    @patch.object(argparse.ArgumentParser, "error")
    def test_validate_minio_values_missing_credentials(self, mock_error, mock_exists):
        # Test that an error is raised when MinIO credentials are missing
        instance = ConcreteClass()
        args = TestingArgs()
        parser = argparse.ArgumentParser()

        args.minio_username = ""
        args.minio_password = ""
        args.type_of_test = "some_test_type"
        instance.validate_minio_values(args, parser)
        mock_error.assert_called_once()

    @patch("os.path.exists", return_value=False)
    @patch("argparse.ArgumentParser.error")
    def test_validate_minio_values_invalid_path(self, mock_error, mock_exists):
        concrete_class = ConcreteClass()
        testing_args = TestingArgs()
        arg_parser = argparse.ArgumentParser()

        testing_args.minio_username = "username"
        testing_args.minio_password = "password"

        # Set minio_app_path to a path that does not exist
        testing_args.minio_app_path = "/invalid/minio_app_path"

        # Call the method under test
        concrete_class.validate_minio_values(testing_args, arg_parser)

        # Assert that parser.error was called with the correct message
        mock_error.assert_called_once_with(
            "/invalid/minio_app_path does not exist or there is not access to it"
        )

    #### Tests for validate_vdms_values() ####
    @patch("os.path.exists", return_value=True)
    @patch.object(argparse.ArgumentParser, "error")
    def test_validate_vdms_values_valid(self, mock_error, mock_exists):
        # Test that no error is raised when the VMDS app path and config files exist
        instance = ConcreteClass()
        args = TestingArgs()
        parser = argparse.ArgumentParser()

        args.vdms_app_path = "/valid/vdms/path"
        args.config_files_for_vdms = ["/valid/config/file"]
        instance.validate_vdms_values(args, parser)
        mock_exists.assert_has_calls(
            [
                unittest.mock.call("/valid/vdms/path"),
                unittest.mock.call("/valid/config/file"),
            ]
        )
        mock_error.assert_not_called()

    @patch("os.path.exists", return_value=False)
    @patch("argparse.ArgumentParser.error")
    def test_validate_vdms_values_invalid_app_path(self, mock_error, mock_exists):
        instance = ConcreteClass()
        testing_args = TestingArgs()
        arg_parser = argparse.ArgumentParser()

        # Set vdms_app_path to a path that does not exist
        testing_args.vdms_app_path = "/invalid/vdms_app_path"

        # Call the method under test
        instance.validate_vdms_values(testing_args, arg_parser)

        # Assert that parser.error was called with the correct message
        mock_error.assert_called_once_with(
            "/invalid/vdms_app_path does not exist or there is not access to it"
        )

    @patch("os.path.exists", return_value=False)
    @patch.object(argparse.ArgumentParser, "error")
    def test_validate_vdms_values_invalid_config(self, mock_error, mock_exists):
        # Test that an error is raised when a VMDS config file does not exist
        instance = ConcreteClass()
        args = TestingArgs()
        parser = argparse.ArgumentParser()

        args.config_files_for_vdms = ["/invalid/config/file"]
        instance.validate_vdms_values(args, parser)
        mock_exists.assert_called_with("/invalid/config/file")
        mock_error.assert_called_once()

    #### Tests for run_minio_server() ####
    @patch("subprocess.Popen")
    @patch("subprocess.check_call")
    @patch("os.system")
    @patch("builtins.print")
    def test_run_minio_server(
        self, mock_print, mock_system, mock_check_call, mock_popen
    ):
        # Mock the pid attribute of the process created by subprocess.Popen

        instance = ConcreteClass()
        args = TestingArgs()
        args.minio_app_path = "/path/to/minio"
        args.tmp_tests_dir = "/tmp/tests"
        args.minio_tmp_dir_name = "minio_tmp"
        args.minio_port = "9000"
        args.minio_alias_name = "myminio"
        args.minio_username = "miniouser"
        args.minio_password = "miniopass"

        mock_process = MagicMock()
        mock_process.pid = 12345
        mock_popen.return_value = mock_process

        stderrFD = MagicMock()
        stdoutFD = MagicMock()

        instance.run_minio_server(args, stderrFD, stdoutFD)

        # Check if subprocess.Popen was called with the correct arguments
        mock_popen.assert_called_once_with(
            [
                args.minio_app_path,
                "server",
                args.minio_tmp_dir_name,
                "--address",
                f":{args.minio_port}",
            ],
            stdout=stdoutFD,
            stderr=stderrFD,
            text=True,
        )

        # Check if the correct print statements were made
        expected_print_calls = [
            unittest.mock.call("Using MinIO server pid:", 12345),
            unittest.mock.call("Creating buckets for the tests"),
        ]
        mock_print.assert_has_calls(expected_print_calls, any_order=False)

        # Check if the sleep commands were called
        mock_system.assert_has_calls(
            [unittest.mock.call("sleep 5"), unittest.mock.call("sleep 3")]
        )

        # Check if the MinIO client commands were called
        mock_check_call.assert_has_calls(
            [
                unittest.mock.call(
                    [
                        "mc",
                        "alias",
                        "set",
                        f"{args.minio_alias_name}/",
                        f"http://localhost:{args.minio_port}",
                        args.minio_username,
                        args.minio_password,
                    ],
                    stdout=stdoutFD,
                    stderr=stderrFD,
                    text=True,
                ),
                unittest.mock.call(
                    ["mc", "mb", f"{args.minio_alias_name}/minio-bucket"],
                    stdout=stdoutFD,
                    stderr=stderrFD,
                    text=True,
                ),
            ]
        )

    @patch("run_all_tests.os.system")  # Mock os.system to prevent actual sleep
    @patch("run_all_tests.subprocess.Popen")
    @patch("run_all_tests.subprocess.check_call")
    def test_run_minio_server_exception_on_check_call(
        self, mock_check_call, mock_popen, mock_system
    ):
        # Test run_minio_server when an exception occurs in subprocess.check_call
        mock_popen.return_value = MagicMock(
            pid=123
        )  # Mock Popen to return a process with pid 123
        mock_check_call.side_effect = subprocess.CalledProcessError(
            1, "mc"
        )  # Simulate an exception in check_call
        instance = ConcreteClass()
        testingArgs = TestingArgs()
        stderrFD = MagicMock()
        stdoutFD = MagicMock()

        with self.assertRaises(Exception) as context:
            instance.run_minio_server(testingArgs, stderrFD, stdoutFD)
        # Assert the exception message is as expected
        self.assertTrue("run_minio_server() error: " in str(context.exception))

    #### Tests for run_vdms_server() ####
    @patch("subprocess.Popen")
    @patch("os.system")
    @patch("builtins.print")
    def test_run_vdms_server(self, mock_print, mock_system, mock_popen):
        # Mock the pid attribute of the process created by subprocess.Popen
        instance = ConcreteClass()
        args = TestingArgs()
        args.vdms_app_path = "/path/to/vdms"
        args.tmp_config_files_for_vdms = ["/tmp/tests/config1", "/tmp/tests/config2"]

        mock_process = MagicMock()
        mock_process.pid = 12345
        mock_popen.return_value = mock_process

        stderrFD = MagicMock()
        stdoutFD = MagicMock()

        instance.run_vdms_server(args, stderrFD, stdoutFD)

        # Check if subprocess.Popen was called with the correct arguments for each config file
        expected_calls = [
            unittest.mock.call(
                [args.vdms_app_path, "-cfg", config],
                stdout=stdoutFD,
                stderr=stderrFD,
                text=True,
            )
            for config in args.tmp_config_files_for_vdms
        ]
        mock_popen.assert_has_calls(expected_calls, any_order=False)

        # Check if the correct print statements were made
        expected_print_calls = []
        for config in args.tmp_config_files_for_vdms:
            expected_print_calls.append(unittest.mock.call("Using config:", config))
            expected_print_calls.append(unittest.mock.call("Using VDMS pid:", 12345))

        mock_print.assert_has_calls(expected_print_calls, any_order=False)

        # Check if the sleep command was called
        mock_system.assert_called_with("sleep 3")

    def test_run_vdms_server_invalid_config_files(self):
        instance = ConcreteClass()
        args = TestingArgs()
        stderrFD = MagicMock()
        stdoutFD = MagicMock()

        # Set tmp_config_files_for_vdms to None to simulate an invalid value
        args.tmp_config_files_for_vdms = None

        # Call the method under test and assert that an exception is raised
        with self.assertRaises(Exception) as context:
            instance.run_vdms_server(args, stderrFD, stdoutFD)

        # Assert that the exception message is as expected
        self.assertIn(
            "run_vdms_server(): tmp_config_files_for_vdms has an invalid value",
            str(context.exception),
        )

    #### Tests for run_google_tests() ####
    @patch("run_all_tests.subprocess.run")
    @patch("run_all_tests.print")
    def test_run_google_tests(self, mock_print, mock_run):
        instance = ConcreteClass()
        run_all_tests.DEBUG_MODE = True

        # Set up TestingArgs
        testing_args = TestingArgs()
        testing_args.stop_tests_on_failure = True
        testing_args.googletest_path = "/path/to/gtest"
        testing_args.test_name = "MyTest"

        # Mock file descriptors
        stderrFD = MagicMock()
        stdoutFD = MagicMock()

        # Call the method
        instance.run_google_tests(testing_args, stderrFD, stdoutFD)

        # Check if the correct print statements were made
        mock_print.assert_any_call("Starting Google tests: MyTest...")


    @patch("run_all_tests.subprocess.Popen")
    @patch("run_all_tests.print")
    def test_run_google_tests_with_exception(self, mock_print, mock_popen):
        instance = ConcreteClass()
        # Set up the mock to raise an exception
        mock_popen.side_effect = Exception("Test Exception")

        # Set up TestingArgs
        testing_args = TestingArgs()
        testing_args.googletest_path = "/path/to/gtest"
        testing_args.test_name = "MyTest"

        # Mock file descriptors
        stderrFD = MagicMock()
        stdoutFD = MagicMock()

        # Mock write_to_fd method
        instance.write_to_fd = MagicMock()

        # Call the method and check for exception
        with self.assertRaises(Exception) as context:
            instance.run_google_tests(testing_args, stderrFD, stdoutFD)

        # Check the exception message
        self.assertTrue("run_google_tests() error:" in str(context.exception))

    #### Tests for open_log_files() ####
    @patch("builtins.open", new_callable=mock_open)
    def test_open_log_files(self, mock_file):
        instance = ConcreteClass()
        args = TestingArgs()
        args.tmp_tests_dir = "/tmp/tests"
        args.stderr_filename = "stderr.log"
        args.stdout_filename = "stdout.log"

        _, _ = instance.open_log_files(
            args.tmp_tests_dir, args.stderr_filename, args.stdout_filename
        )

        # Check if files were opened with the correct arguments
        mock_file.assert_any_call(f"{args.tmp_tests_dir}/{args.stderr_filename}", "w")
        mock_file.assert_any_call(f"{args.tmp_tests_dir}/{args.stdout_filename}", "w")

    #### Tests for set_values_for_python_client() ####
    @patch("run_all_tests.os.path.exists", return_value=True)
    @patch("run_all_tests.print")
    def test_set_values_for_python_client(self, mock_print, mock_exists):
        # Set the constants for the test
        run_all_tests.DEFAULT_DIR_REPO = DEFAULT_DIR_REPO
        run_all_tests.DEBUG_MODE = True

        # Use patch.dict to mock os.environ
        with patch.dict("os.environ", {}, clear=True):
            # Call the method
            instance = ConcreteClass()
            instance.set_values_for_python_client()

            # Check if PYTHONPATH was set correctly
            client_path = f"{DEFAULT_DIR_REPO}/client/python"
            tests_path = f"{DEFAULT_DIR_REPO}/tests/python"
            expected_pythonpath = f"{client_path}:{tests_path}"
            self.assertEqual(os.environ["PYTHONPATH"], expected_pythonpath)

            # Check if the correct print statement was made
            mock_print.assert_called_with("PYTHONPATH:", expected_pythonpath)

    @patch("run_all_tests.os.path.exists", return_value=True)
    @patch.dict("os.environ", {"PYTHONPATH": "/existing/path"}, clear=False)
    def test_set_values_for_python_client_pythonpath_not_none(self, mock_exists):
        run_all_tests.DEFAULT_DIR_REPO = "/path/to/repo"
        run_all_tests.DEBUG_MODE = True
        instance = ConcreteClass()
        # Call the method
        instance.set_values_for_python_client()

        # Check if PYTHONPATH was appended correctly
        existing_pythonpath = "/existing/path"
        client_path = f"{run_all_tests.DEFAULT_DIR_REPO}/client/python"
        tests_path = f"{run_all_tests.DEFAULT_DIR_REPO}/tests/python"
        expected_pythonpath = f"{existing_pythonpath}:{client_path}:{tests_path}"
        self.assertEqual(os.environ["PYTHONPATH"], expected_pythonpath)

    @patch("run_all_tests.os.path.exists", return_value=False)
    @patch.dict(
        "os.environ", {}, clear=True
    )  # Clear the environment variables for the test
    def test_set_values_for_python_client_exception(self, mock_exists):
        instance = ConcreteClass()
        # Define DEFAULT_DIR_REPO for the test
        run_all_tests.DEFAULT_DIR_REPO = "/path/to/repo"

        # Call the method and expect an exception
        with self.assertRaises(Exception) as context:
            instance.set_values_for_python_client()

        # Check that the exception message is correct
        expected_message = "Path to the Python client: /path/to/repo/client/python is invalid or you don't have the permissions to access it"
        self.assertEqual(str(context.exception), expected_message)

    #### run_prep_certs_script ####
    @patch("run_all_tests.os.path.exists", return_value=True)
    @patch("run_all_tests.subprocess.check_call")
    @patch("run_all_tests.print")
    def test_run_prep_certs_script(self, mock_print, mock_check_call, mock_exists):
        # Mock file descriptors
        stderrFD = MagicMock()
        stdoutFD = MagicMock()

        instance = ConcreteClass()
        # Define DEFAULT_DIR_REPO for the test
        run_all_tests.DEFAULT_DIR_REPO = "/path/to/repo"

        # Call the method
        instance.run_prep_certs_script(stderrFD, stdoutFD)

        # Check if the correct print statement was made
        mock_print.assert_called_with("run_prep_certs_script...")

        # Check if the subprocess.check_call was called with the correct arguments
        prepCerts = f"{run_all_tests.DEFAULT_DIR_REPO}/tests/tls_test/prep_certs.py"
        mock_check_call.assert_called_once_with(
            f"python3 {prepCerts}",
            shell=True,
            stderr=stderrFD,
            stdout=stdoutFD,
            text=True,
        )

    @patch("run_all_tests.os.path.exists", return_value=False)
    @patch("run_all_tests.print")
    def test_run_prep_certs_script_invalid_file(self, mock_print, mock_exists):
        # Mock file descriptors
        stderrFD = MagicMock()
        stdoutFD = MagicMock()
        instance = ConcreteClass()
        # Define DEFAULT_DIR_REPO for the test
        run_all_tests.DEFAULT_DIR_REPO = "/path/to/repo"

        # Call the method and check for exception
        with self.assertRaises(Exception) as context:
            instance.run_prep_certs_script(stderrFD, stdoutFD)

        # Check the exception message
        prepCerts = f"{run_all_tests.DEFAULT_DIR_REPO}/tests/tls_test/prep_certs.py"
        expected_message = (
            f"run_prep_certs_script() error: {prepCerts} is an invalid file"
        )
        self.assertEqual(str(context.exception), expected_message)

    #### Tests for run_python_tests() ####
    @patch("run_all_tests.subprocess.Popen")
    @patch("run_all_tests.print")
    def test_run_python_tests(self, mock_print, mock_popen):
        instance = ConcreteClass()
        # Set up the mock for Popen
        mock_process = MagicMock()
        mock_process.communicate.return_value = (b"output", b"errors")
        mock_process.pid = 12345
        mock_popen.return_value = mock_process

        # Set up TestingArgs
        testing_args = TestingArgs()
        testing_args.test_name = "test_module.TestClass"

        # Mock file descriptors
        stderrFD = MagicMock()
        stdoutFD = MagicMock()

        # Mock write_to_fd method
        instance.write_to_fd = MagicMock()

        # Call the method
        instance.run_python_tests(testing_args, stderrFD, stdoutFD)

        # Check if the correct print statements were made
        mock_print.assert_any_call("Running Python tests...")
        mock_print.assert_any_call("Test filter:", testing_args.test_name)

        # Check if the process was added to the list
        self.assertIn(mock_process, run_all_tests.processList)

        # Check if the output and errors were handled correctly
        instance.write_to_fd.assert_any_call(
            "stdout", "run_python_tests", "output", stdoutFD, run_all_tests.DEBUG_MODE
        )
        instance.write_to_fd.assert_any_call(
            "stderr", "run_python_tests", "errors", stderrFD, run_all_tests.DEBUG_MODE
        )

    @patch("run_all_tests.subprocess.Popen")
    def test_run_python_tests_exception(self, mock_popen):
        instance = ConcreteClass()

        # Mock DEFAULT_DIR_REPO
        run_all_tests.DEFAULT_DIR_REPO = "/path/to/repo"

        # Set DEBUG_MODE for testing purposes
        run_all_tests.DEBUG_MODE = True

        # Set up the mock for Popen to return a mock process with a kill method
        mock_process = MagicMock()
        mock_process.communicate.return_value = (b"output", b"errors")
        mock_process.pid = 12345
        mock_popen.return_value = mock_process

        # Set up TestingArgs
        testing_args = TestingArgs()
        testing_args.test_name = "test_module.TestClass"

        # Mock file descriptors
        stderrFD = MagicMock()
        stdoutFD = MagicMock()

        # Mock write_to_fd method
        instance.write_to_fd = MagicMock()

        # Set up the mock to raise an exception after Popen is called
        mock_process.communicate.side_effect = Exception("Test Exception")

        # Call the method and check for exception
        with self.assertRaises(Exception) as context:
            instance.run_python_tests(testing_args, stderrFD, stdoutFD)

        # Check the exception message
        self.assertTrue("run_python_tests() error:" in str(context.exception))

        # Check if the process's kill method was called
        mock_process.kill.assert_called()

    #### Tests for write_to_fd() ####
    @patch("run_all_tests.print")
    def test_write_to_fd_verbose(self, mock_print):
        instance = ConcreteClass()

        # Mock writer object
        writer = MagicMock()

        # Call the method with verbose=True
        message = "Test message"
        instance.write_to_fd("stdout", "write_to_fd", message, writer, verbose=True)

        # Check if the correct print statements were made
        expected_beginning_print = (
            "**************Beginning of stdout logs in write_to_fd()*************"
        )
        expected_end_print = (
            "**************End of stdout logs in write_to_fd()*******************"
        )
        mock_print.assert_any_call(expected_beginning_print)
        mock_print.assert_any_call(message)
        mock_print.assert_any_call(expected_end_print)

        # Check if the writer's write method was called with the correct message
        writer.write.assert_called_once_with(message)

    @patch("run_all_tests.print")
    def test_write_to_fd_not_verbose(self, mock_print):
        instance = ConcreteClass()

        # Mock writer object
        writer = MagicMock()

        # Call the method with verbose=False
        message = "Test message"
        instance.write_to_fd("stdout", "write_to_fd", message, writer, verbose=False)

        # Check if the print statement was made only for the message
        mock_print.assert_called_once_with(message)

        # Check if the writer's write method was called with the correct message
        writer.write.assert_called_once_with(message)

    @patch("run_all_tests.print")
    def test_write_to_fd_exception(self, mock_print):
        instance = ConcreteClass()

        # Mock writer object to raise an exception when write is called
        writer = MagicMock()
        writer.write.side_effect = Exception("Write failed")

        # Call the method and check for exception
        message = "Test message"
        with self.assertRaises(Exception) as context:
            instance.write_to_fd(
                "stdout", "write_to_fd", message, writer, verbose=False
            )

        # Check the exception message
        self.assertTrue("write_to_fd() error: Write failed" in str(context.exception))


#### Tests for the Neo4jTest class
class TestNeo4jTest(unittest.TestCase):

    def setUp(self):
        # Set up for each test
        self.original_DEBUG_MODE = run_all_tests.DEBUG_MODE
        run_all_tests.DEBUG_MODE = True

    def tearDown(self):
        # Clean up after each test
        run_all_tests.DEBUG_MODE = self.original_DEBUG_MODE

    #### Tests for validate_neo4j_values() ####
    @patch.object(argparse.ArgumentParser, "error")
    def test_validate_neo4j_values_missing_username_and_password(self, mock_error):
        # Test that the parser's error method is called when both neo4j_username and
        # neo4j_password are missing or empty.

        neo4j_test = Neo4jTest()
        testing_args = TestingArgs()

        testing_args.neo4j_username = ""
        testing_args.neo4j_password = ""
        testing_args.type_of_test = "neo"

        parser = argparse.ArgumentParser()
        neo4j_test.validate_neo4j_values(testing_args, parser)

        mock_error.assert_called_once()

    @patch.object(argparse.ArgumentParser, "error")
    def test_validate_neo4j_values_missing_username(self, mock_error):
        # Test that the parser's error method is called when neo4j_username is missing or empty.
        neo4j_test = Neo4jTest()
        testing_args = TestingArgs()

        testing_args.neo4j_username = ""
        testing_args.neo4j_password = "some_password"
        testing_args.type_of_test = "neo"

        parser = argparse.ArgumentParser()
        neo4j_test.validate_neo4j_values(testing_args, parser)

        mock_error.assert_called_once()

    @patch.object(argparse.ArgumentParser, "error")
    def test_validate_neo4j_values_missing_password(self, mock_error):
        # Test that the parser's error method is called when neo4j_password is missing or empty.
        neo4j_test = Neo4jTest()
        testing_args = TestingArgs()

        testing_args.neo4j_username = "some_username"
        testing_args.neo4j_password = None
        testing_args.type_of_test = "neo"

        parser = argparse.ArgumentParser()
        neo4j_test.validate_neo4j_values(testing_args, parser)

        mock_error.assert_called_once()

    @patch.object(argparse.ArgumentParser, "error")
    def test_validate_neo4j_values_present(self, mock_error):
        # Test that the parser's error method is not called when both neo4j_username and
        # neo4j_password are provided and not empty.

        neo4j_test = Neo4jTest()
        testing_args = TestingArgs()

        testing_args.neo4j_username = "some_username"
        testing_args.neo4j_password = "some_password"
        testing_args.type_of_test = "neo"

        parser = argparse.ArgumentParser()
        neo4j_test.validate_neo4j_values(testing_args, parser)

        mock_error.assert_not_called()

    #### Tests for get_type_of_neo_test()
    def test_neo4j_ops_io_coordinator_test(self):
        neo4j_test = Neo4jTest()

        self.assertEqual(
            neo4j_test.get_type_of_neo_test("OpsIOCoordinatorTest.SomeTest"),
            NEO4J_OPS_IO_TEST_TYPE,
        )
        self.assertEqual(
            neo4j_test.get_type_of_neo_test("OpsIOCoordinatorTest.*"),
            NEO4J_OPS_IO_TEST_TYPE,
        )
        self.assertEqual(
            neo4j_test.get_type_of_neo_test("'OpsIOCoordinatorTest.SomeTest'"),
            NEO4J_OPS_IO_TEST_TYPE,
        )
        self.assertEqual(
            neo4j_test.get_type_of_neo_test("'OpsIOCoordinatorTest.*'"),
            NEO4J_OPS_IO_TEST_TYPE,
        )

    def test_neo4j_e2e_test(self):
        neo4j_test = Neo4jTest()
        self.assertEqual(
            neo4j_test.get_type_of_neo_test("Neo4JE2ETest.SomeTest"),
            NEO4J_E2E_TEST_TYPE,
        )
        self.assertEqual(
            neo4j_test.get_type_of_neo_test("Neo4JE2ETest.*"),
            NEO4J_E2E_TEST_TYPE,
        )
        self.assertEqual(
            neo4j_test.get_type_of_neo_test("'Neo4JE2ETest.SomeTest'"),
            NEO4J_E2E_TEST_TYPE,
        )
        self.assertEqual(
            neo4j_test.get_type_of_neo_test("'Neo4JE2ETest.*'"),
            NEO4J_E2E_TEST_TYPE,
        )

    def test_neo4j_backend_test(self):
        neo4j_test = Neo4jTest()
        self.assertEqual(
            neo4j_test.get_type_of_neo_test("Neo4jBackendTest.SomeTest"),
            NEO4J_BACKEND_TEST_TYPE,
        )
        self.assertEqual(
            neo4j_test.get_type_of_neo_test("Neo4jBackendTest.*"),
            NEO4J_BACKEND_TEST_TYPE,
        )
        self.assertEqual(
            neo4j_test.get_type_of_neo_test("'Neo4jBackendTest.SomeTest'"),
            NEO4J_BACKEND_TEST_TYPE,
        )
        self.assertEqual(
            neo4j_test.get_type_of_neo_test("'Neo4jBackendTest.*'"),
            NEO4J_BACKEND_TEST_TYPE,
        )

    def test_neo4j_unknown_test(self):
        neo4j_test = Neo4jTest()
        self.assertIsNone(neo4j_test.get_type_of_neo_test("UnknownTest.SomeTest"))
        self.assertIsNone(neo4j_test.get_type_of_neo_test("UnknownTest.*"))
        self.assertIsNone(neo4j_test.get_type_of_neo_test("'UnknownTest.SomeTest'"))
        self.assertIsNone(neo4j_test.get_type_of_neo_test("'UnknownTest.*'"))
        self.assertIsNone(neo4j_test.get_type_of_neo_test("''"))
        self.assertIsNone(neo4j_test.get_type_of_neo_test(""))

    #### Tests for fill_default_arguments() ####
    @patch.object(Neo4jTest, "get_valid_neo4j_values")
    @patch.object(Neo4jTest, "get_valid_google_test_values")
    @patch.object(Neo4jTest, "get_valid_minio_values")
    @patch.object(Neo4jTest, "get_valid_vdms_values")
    @patch.object(Neo4jTest, "get_valid_test_name")
    def test_neo4j_fill_default_arguments_Neo4JE2ETest(
        self,
        mock_get_valid_test_name,
        mock_get_valid_vdms_values,
        mock_get_valid_minio_values,
        mock_get_valid_google_test_values,
        mock_get_valid_neo4j_values,
    ):
        neo4j_test = Neo4jTest()
        testing_args = TestingArgs()
        # Set the neo4j_port to a non-None value
        testing_args.neo4j_port = 7687
        testing_args.neo4j_endpoint = (
            f"neo4j://anyaddress:{str(testing_args.neo4j_port)}"
        )
        testing_args.neo4j_username = "username_test"
        testing_args.neo4j_password = "username_pwd"

        # Set up the mocks with return values
        mock_get_valid_test_name.return_value = "Neo4JE2ETest.default"
        mock_get_valid_vdms_values.return_value = (
            "/vdms/app/path",
            ["config1.json", "config2.json"],
        )
        mock_get_valid_minio_values.return_value = testing_args
        mock_get_valid_google_test_values.return_value = testing_args
        mock_get_valid_neo4j_values.return_value = testing_args

        # Use patch.dict to mock os.environ
        with patch.object(
            neo4j_test, "get_type_of_neo_test", return_value=NEO4J_E2E_TEST_TYPE
        ) as mock_get_type_of_neo_test, patch.dict(
            "os.environ", {}, clear=True
        ) as mock_environ:
            # Call the method under test
            result = neo4j_test.fill_default_arguments(testing_args)

            # Assertions to check if the default values are set correctly
            self.assertEqual(result.test_name, "Neo4JE2ETest.default")
            self.assertEqual(result.vdms_app_path, "/vdms/app/path")
            self.assertEqual(
                result.config_files_for_vdms, ["config1.json", "config2.json"]
            )
            self.assertEqual(result.minio_alias_name, "e2e_tester")

            # Check if environment variables are set
            self.assertEqual(mock_environ["NEO_TEST_PORT"], str(result.neo4j_port))
            self.assertEqual(mock_environ["NEO4J_USER"], result.neo4j_username)
            self.assertEqual(mock_environ["NEO4J_PASS"], result.neo4j_password)
            self.assertEqual(mock_environ["NEO4J_ENDPOINT"], result.neo4j_endpoint)

            # Check that the mocked methods were called
            mock_get_valid_test_name.assert_called_once_with(
                testing_args, DEFAULT_NEO4J_E2E_TEST_FILTER
            )
            mock_get_valid_vdms_values.assert_called_once_with(
                testing_args, DEFAULT_NEO4J_E2E_CONFIG_FILES
            )
            mock_get_valid_minio_values.assert_called_once_with(testing_args)
            mock_get_valid_google_test_values.assert_called_once_with(testing_args)
            mock_get_valid_neo4j_values.assert_called_once_with(testing_args)
            mock_get_type_of_neo_test.assert_called_once()

    @patch.object(Neo4jTest, "get_valid_google_test_values")
    @patch.object(Neo4jTest, "get_valid_minio_values")
    @patch.object(Neo4jTest, "get_valid_test_name")
    def test_neo4j_fill_default_arguments_OpsIOCoordinatorTest(
        self,
        mock_get_valid_test_name,
        mock_get_valid_minio_values,
        mock_get_valid_google_test_values,
    ):
        neo4j_test = Neo4jTest()
        testing_args = TestingArgs()

        # Set up the mocks with return values
        mock_get_valid_test_name.return_value = "OpsIOCoordinatorTest.default"
        mock_get_valid_minio_values.return_value = testing_args
        mock_get_valid_google_test_values.return_value = testing_args

        # Use patch.dict to mock os.environ
        with patch.object(
            neo4j_test, "get_type_of_neo_test", return_value=NEO4J_OPS_IO_TEST_TYPE
        ) as mock_get_type_of_neo_test, patch.dict(
            "os.environ", {}, clear=True
        ) as mock_environ:
            # Call the method under test
            result = neo4j_test.fill_default_arguments(testing_args)

            # Assertions to check if the default values are set correctly
            self.assertEqual(result.test_name, "OpsIOCoordinatorTest.default")
            self.assertEqual(result.minio_alias_name, "opsio_tester")

            # Check that the mocked methods were called
            mock_get_valid_test_name.assert_called_once_with(
                testing_args, DEFAULT_NEO4J_OPSIO_TEST_FILTER
            )
            mock_get_valid_minio_values.assert_called_once_with(testing_args)
            mock_get_valid_google_test_values.assert_called_once_with(testing_args)

    @patch("run_all_tests.Neo4jTest.get_valid_test_name")
    @patch("run_all_tests.Neo4jTest.get_valid_vdms_values")
    @patch("run_all_tests.Neo4jTest.get_valid_minio_values")
    @patch("run_all_tests.Neo4jTest.get_valid_google_test_values")
    @patch("run_all_tests.Neo4jTest.get_valid_neo4j_values")
    def test_fill_default_arguments_for_neo4j_backend_test(
        self,
        mock_get_valid_neo4j_values,
        mock_get_valid_google_test_values,
        mock_get_valid_minio_values,
        mock_get_valid_vdms_values,
        mock_get_valid_test_name,
    ):
        # Test fill_default_arguments method for "Neo4jBackendTest.*" test name
        testingArgs = run_all_tests.TestingArgs()
        testingArgs.test_name = "Neo4jBackendTest.*"

        testingArgs.neo4j_port = 10000
        testingArgs.neo4j_endpoint = f"neo4j://anyaddress:{str(testingArgs.neo4j_port)}"
        testingArgs.neo4j_username = "Neo4jBackendTest_username"
        testingArgs.neo4j_password = "Neo4jBackendTest_password"

        instance = run_all_tests.Neo4jTest()

        # Mock the methods called within fill_default_arguments to return the testingArgs without modification
        mock_get_valid_test_name.return_value = testingArgs.test_name
        mock_get_valid_vdms_values.return_value = (MagicMock(), [])
        mock_get_valid_minio_values.return_value = testingArgs
        mock_get_valid_google_test_values.return_value = testingArgs
        mock_get_valid_neo4j_values.return_value = testingArgs

        result = None
        with patch.object(
            instance, "get_type_of_neo_test", return_value=NEO4J_BACKEND_TEST_TYPE
        ) as mock_get_type_of_neo_test:
            # Call the method under test
            result = instance.fill_default_arguments(testingArgs)
            mock_get_type_of_neo_test.assert_called_once()

        # Assert that config_files_for_vdms is set to an empty list
        self.assertEqual(result.config_files_for_vdms, [])
        # Assert that environment variables are set
        self.assertEqual(os.environ["NEO_TEST_PORT"], str(testingArgs.neo4j_port))
        self.assertEqual(os.environ["NEO4J_USER"], testingArgs.neo4j_username)
        self.assertEqual(os.environ["NEO4J_PASS"], testingArgs.neo4j_password)
        self.assertEqual(os.environ["NEO4J_ENDPOINT"], testingArgs.neo4j_endpoint)

        # Assert that the mocked methods were called
        mock_get_valid_test_name.assert_called_once_with(
            testingArgs, run_all_tests.DEFAULT_NEO4J_OPSIO_TEST_FILTER
        )
        mock_get_valid_vdms_values.assert_not_called()  # Should not be called for "Neo4jBackendTest.*"
        mock_get_valid_minio_values.assert_not_called()  # Should not be called for "Neo4jBackendTest.*"
        mock_get_valid_google_test_values.assert_called_once_with(testingArgs)
        mock_get_valid_neo4j_values.assert_called_once_with(testingArgs)

    @patch("run_all_tests.Neo4jTest.get_valid_test_name")
    @patch("run_all_tests.Neo4jTest.get_valid_vdms_values")
    @patch("run_all_tests.Neo4jTest.get_valid_minio_values")
    @patch("run_all_tests.Neo4jTest.get_valid_google_test_values")
    def test_fill_default_arguments_for_invalid_test_name(
        self,
        mock_get_valid_google_test_values,
        mock_get_valid_minio_values,
        mock_get_valid_vdms_values,
        mock_get_valid_test_name,
    ):
        # Test fill_default_arguments method for "Neo4jBackendTest.*" test name
        testingArgs = run_all_tests.TestingArgs()
        testingArgs.test_name = "InvalidNeo4jTestname.*"

        testingArgs.neo4j_port = 10000
        testingArgs.neo4j_username = "Neo4jBackendTest_username"
        testingArgs.neo4j_password = "Neo4jBackendTest_password"

        instance = run_all_tests.Neo4jTest()

        # Mock the methods called within fill_default_arguments to return the testingArgs without modification
        mock_get_valid_test_name.return_value = testingArgs.test_name
        mock_get_valid_vdms_values.return_value = (MagicMock(), [])
        mock_get_valid_minio_values.return_value = testingArgs
        mock_get_valid_google_test_values.return_value = testingArgs

        result = None
        with patch.object(
            instance, "get_type_of_neo_test", return_value=None
        ) as mock_get_type_of_neo_test:
            # Call the method under test
            result = instance.fill_default_arguments(testingArgs)
            mock_get_type_of_neo_test.assert_called_once()

        # Assert that config_files_for_vdms is set to an empty list
        self.assertEqual(result.config_files_for_vdms, [])

        # Assert that the mocked methods were called
        mock_get_valid_test_name.assert_called_once_with(
            testingArgs, run_all_tests.DEFAULT_NEO4J_OPSIO_TEST_FILTER
        )
        mock_get_valid_vdms_values.assert_not_called()  # Should not be called for "Neo4jBackendTest.*"
        mock_get_valid_minio_values.assert_not_called()  # Should not be called for "Neo4jBackendTest.*"
        mock_get_valid_google_test_values.assert_called_once_with(testingArgs)

    @patch(
        "run_all_tests.Neo4jTest.get_valid_test_name",
        side_effect=Exception("Mocked get_valid_test_name exception"),
    )
    def test_Neo4jTest_fill_default_arguments_exception(self, mock_get_valid_test_name):
        neo4j_test = Neo4jTest()
        testing_args = TestingArgs()

        # Set necessary attributes for fill_default_arguments
        testing_args.test_name = "Neo4jBackendTest."
        testing_args.neo4j_username = "neo4j_user"
        testing_args.neo4j_password = "neo4j_pass"
        testing_args.neo4j_port = 7687

        # Call the method under test and assert that an exception is raised
        with self.assertRaises(Exception) as context:
            neo4j_test.fill_default_arguments(testing_args)

        # Assert that the exception message is as expected
        self.assertIn(
            "fill_default_arguments() in Neo4jTest() error: Mocked get_valid_test_name exception",
            str(context.exception),
        )

    #### Tests for validate_arguments() ####
    @patch("run_all_tests.Neo4jTest.validate_google_test_path")
    @patch("run_all_tests.Neo4jTest.validate_minio_values")
    @patch("run_all_tests.Neo4jTest.validate_vdms_values")
    @patch("run_all_tests.Neo4jTest.validate_neo4j_values")
    def test_Neo4jTest_validate_arguments_Neo4jBackendTest_valid(
        self,
        mock_validate_neo4j,
        mock_validate_vdms,
        mock_validate_minio,
        mock_validate_google,
    ):
        neo4j_test = Neo4jTest()
        testing_args = TestingArgs()
        arg_parser = argparse.ArgumentParser()

        # Set necessary attributes for validate_arguments
        testing_args.test_name = "Neo4jBackendTest."
        testing_args.neo4j_username = "neo4j_user"
        testing_args.neo4j_password = "neo4j_pass"

        # Call the method under test
        try:
            with patch.object(
                neo4j_test, "get_type_of_neo_test", return_value=NEO4J_BACKEND_TEST_TYPE
            ) as mock_get_type_of_neo_test:
                neo4j_test.validate_arguments(testing_args, arg_parser)
                mock_get_type_of_neo_test.assert_called_once()
        except Exception as e:
            self.fail(f"validate_arguments method raised an exception: {e}")

        # Assert that the validation methods were called as expected
        mock_validate_google.assert_called_once_with(testing_args, arg_parser)
        mock_validate_minio.assert_not_called()
        mock_validate_vdms.assert_not_called()
        mock_validate_neo4j.assert_called_once_with(testing_args, arg_parser)

    @patch("run_all_tests.Neo4jTest.validate_google_test_path")
    @patch("run_all_tests.Neo4jTest.validate_minio_values")
    @patch("run_all_tests.Neo4jTest.validate_vdms_values")
    @patch("run_all_tests.Neo4jTest.validate_neo4j_values")
    def test_Neo4jTest_validate_arguments_ops_io_coordinator_test(
        self,
        mock_validate_neo4j,
        mock_validate_vdms,
        mock_validate_minio,
        mock_validate_google,
    ):
        neo4j_test = Neo4jTest()
        testing_args = TestingArgs()
        arg_parser = argparse.ArgumentParser()

        # Set test_name to "OpsIOCoordinatorTest."
        testing_args.test_name = "OpsIOCoordinatorTest."

        with patch.object(
            neo4j_test, "get_type_of_neo_test", return_value=NEO4J_OPS_IO_TEST_TYPE
        ) as mock_get_type_of_neo_test:
            # Call the method under test
            neo4j_test.validate_arguments(testing_args, arg_parser)
            mock_get_type_of_neo_test.assert_called_once()

        # Assert that the necessary validation methods were called
        mock_validate_google.assert_called_once_with(testing_args, arg_parser)
        mock_validate_minio.assert_called_once_with(testing_args, arg_parser)
        mock_validate_vdms.assert_not_called()
        mock_validate_neo4j.assert_not_called()

    @patch("run_all_tests.Neo4jTest.validate_google_test_path")
    @patch("run_all_tests.Neo4jTest.validate_minio_values")
    @patch("run_all_tests.Neo4jTest.validate_vdms_values")
    @patch("run_all_tests.Neo4jTest.validate_neo4j_values")
    def test_Neo4jTest_validate_arguments_neo4j_e2e_test(
        self,
        mock_validate_neo4j,
        mock_validate_vdms,
        mock_validate_minio,
        mock_validate_google,
    ):
        neo4j_test = Neo4jTest()
        testing_args = TestingArgs()
        arg_parser = argparse.ArgumentParser()

        # Set test_name to "Neo4JE2ETest."
        testing_args.test_name = "Neo4JE2ETest."

        with patch.object(
            neo4j_test, "get_type_of_neo_test", return_value=NEO4J_E2E_TEST_TYPE
        ) as mock_get_type_of_neo_test:
            # Call the method under test
            neo4j_test.validate_arguments(testing_args, arg_parser)
            mock_get_type_of_neo_test.assert_called_once()

        # Assert that the necessary validation methods were called
        mock_validate_google.assert_called_once_with(testing_args, arg_parser)
        mock_validate_minio.assert_called_once_with(testing_args, arg_parser)
        mock_validate_vdms.assert_called_once_with(testing_args, arg_parser)
        mock_validate_vdms.assert_called_once_with(testing_args, arg_parser)

    def test_Neo4jTest_validate_arguments_parser_none(self):
        # Test that an exception is raised when parser is None

        neo4j_test = Neo4jTest()
        testing_args = TestingArgs()

        with self.assertRaises(Exception) as context:
            neo4j_test.validate_arguments(testing_args, None)
        self.assertIn("parser is None", str(context.exception))

    @patch.object(Neo4jTest, "validate_google_test_path")
    def test_Neo4jTest_validate_arguments_no_test_name(
        self, mock_validate_google_test_path
    ):
        # Test that an exception is raised when test_name is not provided

        neo4j_test = Neo4jTest()
        testing_args = TestingArgs()
        parser = Mock(spec=argparse.ArgumentParser)

        with self.assertRaises(Exception) as context:
            neo4j_test.validate_arguments(testing_args, parser)
        self.assertIn(
            "test_name value was not provided or it is invalid", str(context.exception)
        )

    @patch.object(Neo4jTest, "validate_google_test_path")
    def test_Neo4jTest_validate_arguments_test_name_none(
        self, mock_validate_google_test_path
    ):
        # Test that an exception is raised when test_name is None

        neo4j_test = Neo4jTest()
        testing_args = TestingArgs()
        parser = Mock(spec=argparse.ArgumentParser)

        testing_args.test_name = None
        with self.assertRaises(Exception) as context:
            neo4j_test.validate_arguments(testing_args, parser)
        self.assertIn(
            "test_name value was not provided or it is invalid", str(context.exception)
        )

    @patch.object(Neo4jTest, "validate_google_test_path")
    @patch.object(Neo4jTest, "get_type_of_neo_test", return_value=None)
    def test_Neo4jTest_validate_arguments_invalid_test_name(
        self, mock_get_type_of_neo_test, mock_validate_google_test_path
    ):
        # Test that an exception is raised when test_name has an invalid value

        neo4j_test = Neo4jTest()
        testing_args = TestingArgs()
        parser = Mock(spec=argparse.ArgumentParser)

        testing_args.test_name = "InvalidTestName"
        with self.assertRaises(Exception) as context:
            neo4j_test.validate_arguments(testing_args, parser)
        self.assertIn(
            "test_name value is invalid:InvalidTestName", str(context.exception)
        )
        mock_get_type_of_neo_test.assert_called_once()

    #### Tests for run() ####
    def test_Neo4jTest_run_with_ops_io_coordinator_test_starts_minio_server(self):
        """
        Test that the MinIO server is started when the test_name attribute of TestingArgs
        starts with "OpsIOCoordinatorTest.".
        """
        neo4j_test = Neo4jTest()
        testing_args = TestingArgs()

        testing_args.test_name = "OpsIOCoordinatorTest.some_test_case"
        with patch.object(
            neo4j_test, "open_log_files", return_value=(MagicMock(), MagicMock())
        ), patch.object(
            neo4j_test, "get_type_of_neo_test", return_value=NEO4J_OPS_IO_TEST_TYPE
        ), patch.object(
            neo4j_test, "run_minio_server", return_value=123
        ) as mock_run_minio_server, patch.object(
            neo4j_test, "run_google_tests", return_value=456
        ) as mock_run_google_tests:
            neo4j_test.run(testing_args)
            mock_run_minio_server.assert_called_once()

    # Test that MinIO server starts when test_name starts with "Neo4JE2ETest."
    def test_Neo4jTest_run_starts_minio_server_for_neo4j_e2e_test(self):
        """
        Test that the MinIO server is started when the test_name attribute of TestingArgs
        starts with "Neo4JE2ETest.". Also, ensure that VDMS servers are not started since
        this test is only checking the MinIO server startup.
        """
        neo4j_test = Neo4jTest()
        testing_args = TestingArgs()
        testing_args.test_name = "Neo4JE2ETest.some_test_case"

        with patch.object(
            neo4j_test, "open_log_files", return_value=(MagicMock(), MagicMock())
        ), patch.object(
            neo4j_test, "get_type_of_neo_test", return_value=NEO4J_E2E_TEST_TYPE
        ), patch.object(
            neo4j_test, "run_minio_server", return_value=789
        ) as mock_run_minio_server, patch.object(
            neo4j_test, "run_vdms_server"
        ) as mock_run_vdms_server, patch.object(
            neo4j_test, "run_google_tests", return_value=1011
        ) as mock_run_google_tests:
            neo4j_test.run(testing_args)
            mock_run_minio_server.assert_called_once()
            mock_run_vdms_server.assert_called_once()  # Ensure that VDMS servers are not started in this test case

    # Test that VDMS servers are started when test_name starts with "Neo4JE2ETest."
    def test_Neo4jTest_run_starts_vdms_servers_for_neo4j_e2e_test(self):
        """
        Test that VDMS servers are started when the test_name attribute of TestingArgs
        starts with "Neo4JE2ETest." and that multiple PIDs are added to the pidList.
        """
        neo4j_test = Neo4jTest()
        testing_args = TestingArgs()

        testing_args.test_name = "Neo4JE2ETest.some_test_case"
        testing_args.config_files_for_vdms = ["config1", "config2"]
        with patch.object(
            neo4j_test, "open_log_files", return_value=(MagicMock(), MagicMock())
        ), patch.object(
            neo4j_test, "get_type_of_neo_test", return_value=NEO4J_E2E_TEST_TYPE
        ), patch.object(
            neo4j_test, "run_minio_server", return_value=789
        ), patch.object(
            neo4j_test, "run_vdms_server", return_value=[2021, 2022]
        ) as mock_run_vdms_server, patch.object(
            neo4j_test, "run_google_tests", return_value=1011
        ):
            neo4j_test.run(testing_args)
            mock_run_vdms_server.assert_called_once()

    # Test that GoogleTest tests are run for all test names.
    def test_Neo4jTest_run_google_tests_for_all_test_names(self):
        """
        Test that GoogleTest tests are run regardless of the test_name attribute of TestingArgs.
        """
        neo4j_test = Neo4jTest()
        testing_args = TestingArgs()

        # Set the necessary attributes for testingArgs
        testing_args.tmp_tests_dir = "/tmp"
        testing_args.stderr_filename = "stderr.log"
        testing_args.stdout_filename = "stdout.log"
        testing_args.minio_stderr_filename = "minio_stderr.log"
        testing_args.minio_stdout_filename = "minio_stdout.log"
        testing_args.vdms_stderr_filename = "vdms_stderr.log"
        testing_args.vdms_stdout_filename = "vdms_stdout.log"
        testing_args.test_name = "SomeOtherTest.some_test_case"
        testing_args.run = True  # Ensure this attribute is set to True

        with patch.object(
            neo4j_test, "open_log_files", return_value=(MagicMock(), MagicMock())
        ), patch.object(
            neo4j_test, "get_type_of_neo_test", return_value=None
        ), patch.object(
            neo4j_test, "run_google_tests", return_value=3031
        ) as mock_run_google_tests:
            neo4j_test.run(testing_args)
            mock_run_google_tests.assert_called_once()

    # Test exception handling
    def test_Neo4jTest_run_exception(self):
        """
        Test that an exception in the run method is caught and re-raised with the correct message.
        """
        neo4j_test = Neo4jTest()
        testing_args = TestingArgs()

        # Set the necessary attributes for testingArgs
        testing_args.tmp_tests_dir = "/tmp"
        testing_args.stderr_filename = "stderr.log"
        testing_args.stdout_filename = "stdout.log"
        testing_args.minio_stderr_filename = "minio_stderr.log"
        testing_args.minio_stdout_filename = "minio_stdout.log"
        testing_args.vdms_stderr_filename = "vdms_stderr.log"
        testing_args.vdms_stdout_filename = "vdms_stdout.log"
        testing_args.test_name = "SomeOtherTest.some_test_case"
        testing_args.run = True  # Ensure this attribute is set to True

        # Mock the open_log_files method to raise an exception
        with patch.object(
            neo4j_test, "open_log_files", side_effect=Exception("Log file open failed")
        ):
            # Call the run method and check for the raised exception
            with self.assertRaises(Exception) as context:
                neo4j_test.run(testing_args)

            # Check the exception message
            self.assertEqual(
                str(context.exception),
                "run() Exception in Neo4jTest:Log file open failed",
            )


#### Tests for the NonRemoteTest class
class TestNonRemoteTest(unittest.TestCase):

    def setUp(self):
        self.non_remote_test = NonRemoteTest()
        self.original_DEBUG_MODE = run_all_tests.DEBUG_MODE
        run_all_tests.DEBUG_MODE = True

    def tearDown(self):
        # Clean up after each test
        run_all_tests.DEBUG_MODE = self.original_DEBUG_MODE

    # Test setup_requirements_for_remote_udf_server
    @patch("run_all_tests.subprocess.run")
    def test_setup_requirements_for_remote_udf_server(self, mock_run):
        # Mock file descriptors
        stderrFD = MagicMock()
        stdoutFD = MagicMock()

        # Call the method
        self.non_remote_test.setup_requirements_for_remote_udf_server(
            stderrFD, stdoutFD
        )

    @patch(
        "run_all_tests.subprocess.Popen",
        side_effect=Exception("Test Exception during Popen"),
    )
    def test_setup_requirements_for_remote_udf_server_exception_during_popen(
        self, mock_popen
    ):
        # Mock file descriptors
        stderrFD = MagicMock()
        stdoutFD = MagicMock()

        # Call the method and expect an exception
        with self.assertRaises(Exception) as context:
            self.non_remote_test.setup_requirements_for_remote_udf_server(
                stderrFD, stdoutFD
            )

        # Check the exception message
        expected_message = "setup_requirements_for_remote_udf_server() error: Test Exception during Popen"
        self.assertEqual(str(context.exception), expected_message)

        mock_popen.assert_called_once()

        #### Tests for run_remote_udf_server ####

    @patch("run_all_tests.os.path.exists", return_value=True)
    @patch("run_all_tests.subprocess.Popen")
    @patch("run_all_tests.os.system")
    @patch("run_all_tests.print")
    def test_run_remote_udf_server(
        self, mock_print, mock_system, mock_popen, mock_exists
    ):
        # Set up the mock for Popen
        mock_process = MagicMock()
        mock_process.pid = 12345
        mock_popen.return_value = mock_process

        # Mock file descriptors
        stderrFD = MagicMock()
        stdoutFD = MagicMock()

        # Call the method
        tmp_dir = "/tmp/udf"
        self.non_remote_test.run_remote_udf_server(tmp_dir, stderrFD, stdoutFD)

        # Check if the correct print statement was made
        if run_all_tests.DEBUG_MODE:
            mock_print.assert_called_with("Using python3 pid:", mock_process.pid)

        # Check if the process was added to the list
        self.assertIn(mock_process, run_all_tests.processList)

        # Check if sleep was called to wait for the server initialization
        mock_system.assert_called_once_with("sleep 5")

    @patch("run_all_tests.os.path.exists", return_value=False)
    def test_run_remote_udf_server_invalid_file(self, mock_exists):
        # Mock file descriptors
        stderrFD = MagicMock()
        stdoutFD = MagicMock()

        # Call the method and check for exception
        tmp_dir = "/tmp/udf"
        with self.assertRaises(Exception) as context:
            self.non_remote_test.run_remote_udf_server(tmp_dir, stderrFD, stdoutFD)

        # Check the exception message
        udfServer = (
            f"{run_all_tests.DEFAULT_DIR_REPO}/tests/remote_function_test/udf_server.py"
        )
        expected_message = (
            f"run_remote_udf_server() error: {udfServer} is an invalid file"
        )
        self.assertEqual(str(context.exception), expected_message)

    @patch("run_all_tests.os.path.exists", return_value=True)
    @patch("run_all_tests.subprocess.Popen")
    def test_run_remote_udf_server_exception_after_popen(self, mock_popen, mock_exists):
        # Mock file descriptors
        stderrFD = MagicMock()
        stdoutFD = MagicMock()

        # Mock Popen to raise an exception
        mock_popen.side_effect = Exception("Test Exception during Popen")

        # Call the method and expect an exception
        with self.assertRaises(Exception) as context:
            self.non_remote_test.run_remote_udf_server("/tmp/dir", stderrFD, stdoutFD)

        # Check the exception message
        expected_message = "run_remote_udf_server() error: Test Exception during Popen"
        self.assertEqual(str(context.exception), expected_message)

    #### Tests for setup_for_remote_udf_server_tests ####
    @patch("run_all_tests.NonRemoteTest.setup_requirements_for_remote_udf_server")
    @patch("run_all_tests.NonRemoteTest.run_remote_udf_server")
    @patch("run_all_tests.print")
    def test_setup_for_remote_udf_server_tests_success(
        self,
        mock_print,
        mock_run_remote_udf_server,
        mock_setup_requirements_for_remote_udf_server,
    ):
        # Mock file descriptors
        stderrFD = MagicMock()
        stdoutFD = MagicMock()

        # Call the method
        tmp_dir = "/tmp/udf"
        self.non_remote_test.setup_for_remote_udf_server_tests(
            tmp_dir, stderrFD, stdoutFD
        )

        # Check if the print statement was made
        mock_print.assert_called_with("setup_for_remote_udf_server_tests...")

        # Check if the setup and run methods were called
        mock_setup_requirements_for_remote_udf_server.assert_called_once_with(
            stderrFD, stdoutFD
        )
        mock_run_remote_udf_server.assert_called_once_with(tmp_dir, stderrFD, stdoutFD)

    @patch(
        "run_all_tests.NonRemoteTest.setup_requirements_for_remote_udf_server",
        side_effect=Exception("Setup failed"),
    )
    @patch("run_all_tests.print")
    def test_setup_for_remote_udf_server_tests_exception(
        self, mock_print, mock_setup_requirements_for_remote_udf_server
    ):
        # Mock file descriptors
        stderrFD = MagicMock()
        stdoutFD = MagicMock()

        # Call the method and expect an exception
        tmp_dir = "/tmp/udf"
        with self.assertRaises(Exception) as context:
            self.non_remote_test.setup_for_remote_udf_server_tests(
                tmp_dir, stderrFD, stdoutFD
            )

        # Check the exception message
        expected_message = (
            "setup_for_remote_udf_server_tests() in NonRemoteTest() error: Setup failed"
        )
        self.assertEqual(str(context.exception), expected_message)

    #### Tests for setup_requirements_for_local_udf_message_queue ####
    @patch("run_all_tests.subprocess.run")
    def test_setup_requirements_for_local_udf_message_queue_success(
        self, mock_run
    ):
        nonRemoteTest = NonRemoteTest()

        # Mock file descriptors
        stderrFD = MagicMock()
        stdoutFD = MagicMock()

        # Call the method
        nonRemoteTest.setup_requirements_for_local_udf_message_queue(stderrFD, stdoutFD)

    @patch("run_all_tests.subprocess.Popen", side_effect=Exception("Popen failed"))
    def test_setup_requirements_for_local_udf_message_queue_exception_during_popen(
        self, mock_popen
    ):
        # Mock file descriptors
        stderrFD = MagicMock()
        stdoutFD = MagicMock()

        nonRemoteTest = NonRemoteTest()

        # Call the method and expect an exception
        with self.assertRaises(Exception) as context:
            nonRemoteTest.setup_requirements_for_local_udf_message_queue(
                stderrFD, stdoutFD
            )

        # Check the exception message
        expected_message = (
            "setup_requirements_for_local_udf_message_queue() error: Popen failed"
        )
        self.assertEqual(str(context.exception), expected_message)

    @patch("run_all_tests.subprocess.Popen", side_effect=Exception("Popen failed"))
    def test_setup_requirements_for_local_udf_message_queue_exception_during_popen(
        self, mock_popen
    ):
        # Mock file descriptors
        stderrFD = MagicMock()
        stdoutFD = MagicMock()

        run_all_tests.DEBUG_MODE = True

        nonRemoteTest = NonRemoteTest()

        # Call the method and expect an exception
        with self.assertRaises(Exception) as context:
            nonRemoteTest.setup_requirements_for_local_udf_message_queue(
                stderrFD, stdoutFD
            )

        # Check the exception message
        expected_message = (
            "setup_requirements_for_local_udf_message_queue() error: Popen failed"
        )
        self.assertEqual(str(context.exception), expected_message)

        # Since the exception is raised during Popen, the process object is not created,
        # and therefore, the kill method should not be called. We verify that Popen was called.
        mock_popen.assert_called_once()

    #### Tests for run_local_udf_message_queue ####
    @patch("run_all_tests.os.path.exists", return_value=True)
    @patch("run_all_tests.subprocess.Popen")
    @patch("run_all_tests.print")
    def test_run_local_udf_message_queue_success(
        self, mock_print, mock_popen, mock_exists
    ):
        # Set up the mock for Popen
        mock_process = MagicMock()
        mock_process.pid = 12345
        mock_popen.return_value = mock_process

        run_all_tests.DEBUG_MODE
        nonRemoteTest = NonRemoteTest()

        # Mock file descriptors
        stderrFD = MagicMock()
        stdoutFD = MagicMock()

        # Call the method
        tmp_dir = "/tmp/udf"
        nonRemoteTest.run_local_udf_message_queue(tmp_dir, stderrFD, stdoutFD)

        # Check if the correct print statement was made
        mock_print.assert_called_with("Using python3 pid:", mock_process.pid)

        # Check if the process was added to the list
        self.assertIn(mock_process, run_all_tests.processList)

    @patch("run_all_tests.os.path.exists", return_value=False)
    def test_run_local_udf_message_queue_invalid_file(self, mock_exists):
        # Mock file descriptors
        stderrFD = MagicMock()
        stdoutFD = MagicMock()

        run_all_tests.DEBUG_MODE
        nonRemoteTest = NonRemoteTest()

        # Call the method and expect an exception
        tmp_dir = "/tmp/udf"
        with self.assertRaises(Exception) as context:
            nonRemoteTest.run_local_udf_message_queue(tmp_dir, stderrFD, stdoutFD)

        # Check the exception message
        udfLocal = f"{run_all_tests.DEFAULT_DIR_REPO}/tests/udf_test/udf_local.py"
        expected_message = (
            f"run_local_udf_message_queue() error: {udfLocal} is an invalid file"
        )
        self.assertEqual(str(context.exception), expected_message)

    @patch("run_all_tests.os.path.exists", return_value=True)
    @patch("run_all_tests.subprocess.Popen", side_effect=Exception("Popen failed"))
    def test_run_local_udf_message_queue_exception_during_popen(
        self, mock_popen, mock_exists
    ):
        # Mock file descriptors
        stderrFD = MagicMock()
        stdoutFD = MagicMock()

        run_all_tests.DEBUG_MODE
        nonRemoteTest = NonRemoteTest()

        # Call the method and expect an exception
        tmp_dir = "/tmp/udf"
        with self.assertRaises(Exception) as context:
            nonRemoteTest.run_local_udf_message_queue(tmp_dir, stderrFD, stdoutFD)

        # Check the exception message
        expected_message = "run_local_udf_message_queue() error: Popen failed"
        self.assertEqual(str(context.exception), expected_message)

    #### Tests for setup_for_local_udf_message_queue_tests ####
    @patch("run_all_tests.NonRemoteTest.setup_requirements_for_local_udf_message_queue")
    @patch("run_all_tests.NonRemoteTest.run_local_udf_message_queue")
    @patch("run_all_tests.print")
    def test_setup_for_local_udf_message_queue_tests_success(
        self,
        mock_print,
        mock_run_local_udf_message_queue,
        mock_setup_requirements_for_local_udf_message_queue,
    ):
        # Mock file descriptors
        stderrFD = MagicMock()
        stdoutFD = MagicMock()

        non_remote_test = NonRemoteTest()

        # Call the method
        tmp_dir = "/tmp/udf"
        non_remote_test.setup_for_local_udf_message_queue_tests(
            tmp_dir, stderrFD, stdoutFD
        )

        # Check if the print statement was made
        mock_print.assert_called_with("setup_for_local_udf_message_queue_tests...")

        # Check if the setup and run methods were called
        mock_setup_requirements_for_local_udf_message_queue.assert_called_once_with(
            stderrFD, stdoutFD
        )
        mock_run_local_udf_message_queue.assert_called_once_with(
            tmp_dir, stderrFD, stdoutFD
        )

    @patch(
        "run_all_tests.NonRemoteTest.setup_requirements_for_local_udf_message_queue",
        side_effect=Exception("Setup failed"),
    )
    @patch("run_all_tests.print")
    def test_setup_for_local_udf_message_queue_tests_exception(
        self, mock_print, mock_setup_requirements_for_local_udf_message_queue
    ):
        # Mock file descriptors
        stderrFD = MagicMock()
        stdoutFD = MagicMock()

        non_remote_test = NonRemoteTest()

        # Call the method and expect an exception
        tmp_dir = "/tmp/udf"
        with self.assertRaises(Exception) as context:
            self.non_remote_test.setup_for_local_udf_message_queue_tests(
                tmp_dir, stderrFD, stdoutFD
            )

        # Check the exception message
        expected_message = "setup_for_local_udf_message_queue_tests() in NonRemoteTest() error: Setup failed"
        self.assertEqual(str(context.exception), expected_message)

    #### Tests for fill_default_arguments() ####
    @patch.object(NonRemoteTest, "get_valid_google_test_values")
    @patch.object(NonRemoteTest, "get_valid_vdms_values")
    @patch.object(NonRemoteTest, "get_valid_test_name")
    def test_NonRemoteTest_fill_default_arguments(
        self,
        mock_get_valid_test_name,
        mock_get_valid_vdms_values,
        mock_get_valid_google_test_values,
    ):
        # Set up the mocks with return values
        non_remote_test = NonRemoteTest()
        testing_args = TestingArgs()

        mock_get_valid_test_name.return_value = "provided_test_name"
        mock_get_valid_vdms_values.return_value = (
            "/vdms/app/path",
            ["config1.json", "config2.json"],
        )
        mock_get_valid_google_test_values.return_value = testing_args

        # Call the method under test
        result = non_remote_test.fill_default_arguments(testing_args)

        # Assertions to check if the default values are set correctly
        self.assertEqual(result.test_name, "provided_test_name")
        self.assertEqual(result.vdms_app_path, "/vdms/app/path")
        self.assertEqual(result.config_files_for_vdms, ["config1.json", "config2.json"])

        # Check that the mocked methods were called with the expected arguments
        mock_get_valid_test_name.assert_called_once_with(
            testing_args, DEFAULT_NON_REMOTE_UNIT_TEST_FILTER
        )
        mock_get_valid_vdms_values.assert_called_once_with(
            testing_args, DEFAULT_NON_REMOTE_UNIT_TEST_CONFIG_FILES
        )
        mock_get_valid_google_test_values.assert_called_once_with(testing_args)

    @patch.object(NonRemoteTest, "get_valid_google_test_values")
    @patch.object(NonRemoteTest, "get_valid_vdms_values")
    @patch.object(NonRemoteTest, "get_valid_test_name")
    def test_NonRemoteTest_fill_default_arguments_exception_handling(
        self,
        mock_get_valid_test_name,
        mock_get_valid_vdms_values,
        mock_get_valid_google_test_values,
    ):
        # Set up the mocks to raise an exception
        non_remote_test = NonRemoteTest()
        testing_args = TestingArgs()

        mock_get_valid_test_name.side_effect = Exception("Test exception")

        # Call the method under test and verify that an exception is raised with the correct message
        with self.assertRaises(Exception) as context:
            non_remote_test.fill_default_arguments(testing_args)

        # Check the exception message
        self.assertIn(
            "fill_default_arguments() in NonRemoteTest() error: Test exception",
            str(context.exception),
        )

        # Check that the mocked method that raises the exception was called
        mock_get_valid_test_name.assert_called_once_with(
            testing_args, DEFAULT_NON_REMOTE_UNIT_TEST_FILTER
        )

        # The following methods should not be called since an exception is raised before they are reached
        mock_get_valid_vdms_values.assert_not_called()
        mock_get_valid_google_test_values.assert_not_called()

    #### Tests for validate_arguments() ####
    @patch.object(NonRemoteTest, "validate_google_test_path")
    @patch.object(NonRemoteTest, "validate_vdms_values")
    def test_NonRemoteTest_validate_arguments(
        self, mock_validate_vdms_values, mock_validate_google_test_path
    ):
        # Call the method under test
        non_remote_test = NonRemoteTest()
        testing_args = TestingArgs()
        parser = argparse.ArgumentParser()

        non_remote_test.validate_arguments(testing_args, parser)

        # Check that the validation methods were called with the correct arguments
        mock_validate_vdms_values.assert_called_once_with(testing_args, parser)
        mock_validate_google_test_path.assert_called_once_with(testing_args, parser)

    @patch.object(NonRemoteTest, "validate_google_test_path")
    @patch.object(NonRemoteTest, "validate_vdms_values")
    def test_NonRemoteTest_validate_arguments_with_none_parser(
        self, mock_validate_vdms_values, mock_validate_google_test_path
    ):
        # Call the method under test with a None parser and verify that an exception is raised
        non_remote_test = NonRemoteTest()
        testing_args = TestingArgs()

        with self.assertRaises(Exception) as context:
            non_remote_test.validate_arguments(testing_args, None)

        # Check the exception message
        self.assertIn(
            "validate_arguments() in NonRemoteTest error: parser is None",
            str(context.exception),
        )

        # The validation methods should not be called since an exception is raised before they are reached
        mock_validate_vdms_values.assert_not_called()
        mock_validate_google_test_path.assert_not_called()

    @patch.object(NonRemoteTest, "validate_google_test_path")
    @patch.object(NonRemoteTest, "validate_vdms_values")
    def test_NonRemoteTest_validate_arguments_vdms_values_exception(
        self, mock_validate_vdms_values, mock_validate_google_test_path
    ):
        # Mock validate_vdms_values to raise an exception
        non_remote_test = NonRemoteTest()
        testing_args = TestingArgs()
        parser = argparse.ArgumentParser()

        mock_validate_vdms_values.side_effect = Exception("VDMS validation failed")

        # Call the method under test and verify that an exception is raised
        with self.assertRaises(Exception) as context:
            non_remote_test.validate_arguments(testing_args, parser)

        # Check the exception message
        self.assertIn(
            "validate_arguments() in NonRemoteTest error: VDMS validation failed",
            str(context.exception),
        )

        # Check that validate_vdms_values was called and validate_google_test_path was not called
        mock_validate_vdms_values.assert_called_once_with(testing_args, parser)
        mock_validate_google_test_path.assert_not_called()

    @patch.object(NonRemoteTest, "validate_google_test_path")
    @patch.object(NonRemoteTest, "validate_vdms_values")
    def test_NonRemoteTest_validate_arguments_google_test_path_exception(
        self, mock_validate_vdms_values, mock_validate_google_test_path
    ):
        # Mock validate_google_test_path to raise an exception
        non_remote_test = NonRemoteTest()
        testing_args = TestingArgs()
        parser = argparse.ArgumentParser()

        mock_validate_google_test_path.side_effect = Exception(
            "GoogleTestPath validation failed"
        )

        # Call the method under test and verify that an exception is raised
        with self.assertRaises(Exception) as context:
            non_remote_test.validate_arguments(testing_args, parser)

        # Check the exception message
        self.assertIn(
            "validate_arguments() in NonRemoteTest error: GoogleTestPath validation failed",
            str(context.exception),
        )

        # Check that both validation methods were called
        mock_validate_vdms_values.assert_called_once_with(testing_args, parser)
        mock_validate_google_test_path.assert_called_once_with(testing_args, parser)

    #### Tests for run() ####
    @patch(
        "run_all_tests.NonRemoteTest.open_log_files",
        return_value=(MagicMock(), MagicMock()),
    )
    @patch("run_all_tests.NonRemoteTest.setup_for_remote_udf_server_tests")
    @patch("run_all_tests.NonRemoteTest.setup_for_local_udf_message_queue_tests")
    @patch("run_all_tests.NonRemoteTest.run_prep_certs_script")
    @patch("run_all_tests.NonRemoteTest.run_vdms_server")
    @patch("run_all_tests.NonRemoteTest.run_google_tests")
    @patch("run_all_tests.print")
    def test_run_success(
        self,
        mock_print,
        mock_run_google_tests,
        mock_run_vdms_server,
        mock_run_prep_certs_script,
        mock_setup_for_local_udf_message_queue_tests,
        mock_setup_for_remote_udf_server_tests,
        mock_open_log_files,
    ):
        non_remote_test = NonRemoteTest()
        testing_args = TestingArgs()
        testing_args.tmp_tests_dir = "/tmp/tests"
        testing_args.stderr_filename = "stderr.log"
        testing_args.stdout_filename = "stdout.log"
        testing_args.udf_local_stderr_filename = "udf_local_stderr.log"
        testing_args.udf_local_stdout_filename = "udf_local_stdout.log"
        testing_args.udf_server_stderr_filename = "udf_server_stderr.log"
        testing_args.udf_server_stdout_filename = "udf_server_stdout.log"
        testing_args.tls_stderr_filename = "tls_stderr.log"
        testing_args.tls_stdout_filename = "tls_stdout.log"
        testing_args.vdms_stderr_filename = "vdms_stderr.log"
        testing_args.vdms_stdout_filename = "vdms_stdout.log"
        testing_args.run = True

        # Call the method
        non_remote_test.run(testing_args)

        # Check if the necessary methods were called
        mock_open_log_files.assert_called()
        mock_setup_for_remote_udf_server_tests.assert_called()
        mock_setup_for_local_udf_message_queue_tests.assert_called()
        mock_run_prep_certs_script.assert_called()
        mock_run_vdms_server.assert_called()
        mock_run_google_tests.assert_called()

        # Check if the print statement was made
        mock_print.assert_called_with("Finished")

    @patch(
        "run_all_tests.NonRemoteTest.open_log_files",
        side_effect=Exception("Log file open failed"),
    )
    def test_run_exception(self, mock_open_log_files):
        non_remote_test = NonRemoteTest()
        testing_args = TestingArgs()
        testing_args.tmp_tests_dir = "/tmp/tests"
        testing_args.stderr_filename = "stderr.log"
        testing_args.stdout_filename = "stdout.log"
        testing_args.udf_local_stderr_filename = "udf_local_stderr.log"
        testing_args.udf_local_stdout_filename = "udf_local_stdout.log"
        testing_args.udf_server_stderr_filename = "udf_server_stderr.log"
        testing_args.udf_server_stdout_filename = "udf_server_stdout.log"
        testing_args.tls_stderr_filename = "tls_stderr.log"
        testing_args.tls_stdout_filename = "tls_stdout.log"
        testing_args.vdms_stderr_filename = "vdms_stderr.log"
        testing_args.vdms_stdout_filename = "vdms_stdout.log"
        testing_args.run = True

        # Call the method and expect an exception
        with self.assertRaises(Exception) as context:
            non_remote_test.run(testing_args)

        # Check the exception message
        expected_message = "run() Exception in NonRemoteTest: Log file open failed"
        self.assertEqual(str(context.exception), expected_message)


#### Tests for the NonRemotePythonTest class
class TestNonRemotePythonTest(unittest.TestCase):
    #### Tests for validate_arguments() ####
    @patch.object(NonRemotePythonTest, "validate_google_test_path")
    @patch.object(NonRemotePythonTest, "validate_vdms_values")
    def test_NonRemotePythonTest_validate_arguments(
        self, mock_validate_vdms_values, mock_validate_google_test_path
    ):
        # Call the method under test
        non_remote_python_test = NonRemotePythonTest()
        testing_args = TestingArgs()
        parser = argparse.ArgumentParser()

        non_remote_python_test.validate_arguments(testing_args, parser)

        # Check that the validation methods were called with the correct arguments
        mock_validate_vdms_values.assert_called_once_with(testing_args, parser)
        mock_validate_google_test_path.assert_called_once_with(testing_args, parser)

    def test_NonRemotePythonTest_validate_arguments_with_none_parser(self):
        # Call the method under test with a None parser and verify that an exception is raised
        non_remote_python_test = NonRemotePythonTest()
        testing_args = TestingArgs()
        parser = argparse.ArgumentParser()

        with self.assertRaises(Exception) as context:
            non_remote_python_test.validate_arguments(testing_args, None)

        # Check the exception message
        self.assertIn(
            "validate_arguments() in NonRemotePythonTest error: parser is None",
            str(context.exception),
        )

    @patch.object(NonRemotePythonTest, "validate_google_test_path")
    @patch.object(NonRemotePythonTest, "validate_vdms_values")
    def test_NonRemotePythonTest_validate_arguments_vdms_values_exception(
        self, mock_validate_vdms_values, mock_validate_google_test_path
    ):
        # Mock validate_vdms_values to raise an exception
        non_remote_python_test = NonRemotePythonTest()
        testing_args = TestingArgs()
        parser = argparse.ArgumentParser()

        mock_validate_vdms_values.side_effect = Exception("VDMS validation failed")

        # Call the method under test and verify that an exception is raised
        with self.assertRaises(Exception) as context:
            non_remote_python_test.validate_arguments(testing_args, parser)

        # Check the exception message
        self.assertIn(
            "validate_arguments() in NonRemotePythonTest error: VDMS validation failed",
            str(context.exception),
        )

    @patch.object(NonRemotePythonTest, "validate_google_test_path")
    @patch.object(NonRemotePythonTest, "validate_vdms_values")
    def test_NonRemotePythonTest_validate_arguments_google_test_path_exception(
        self, mock_validate_vdms_values, mock_validate_google_test_path
    ):
        # Mock validate_google_test_path to raise an exception
        non_remote_python_test = NonRemotePythonTest()
        testing_args = TestingArgs()
        parser = argparse.ArgumentParser()

        mock_validate_google_test_path.side_effect = Exception(
            "GoogleTestPath validation failed"
        )

        # Call the method under test and verify that an exception is raised
        with self.assertRaises(Exception) as context:
            non_remote_python_test.validate_arguments(testing_args, parser)

        # Check the exception message
        self.assertIn(
            "validate_arguments() in NonRemotePythonTest error: GoogleTestPath validation failed",
            str(context.exception),
        )

    #### Tests for fill_default_arguments() ####
    @patch.object(NonRemotePythonTest, "get_valid_vdms_values")
    @patch.object(NonRemotePythonTest, "get_valid_test_name")
    def test_non_remote_python_fill_default_arguments(
        self, mock_get_valid_test_name, mock_get_valid_vdms_values
    ):
        # Set up the mocks with return values
        non_remote_python_test = NonRemotePythonTest()
        testing_args = TestingArgs()

        mock_get_valid_test_name.return_value = "provided_test_name"
        mock_get_valid_vdms_values.return_value = (
            "/vdms/app/path",
            ["config1.json", "config2.json"],
        )

        # Call the method under test
        result = non_remote_python_test.fill_default_arguments(testing_args)

        # Assertions to check if the default values are set correctly
        self.assertEqual(result.test_name, "provided_test_name")
        self.assertEqual(result.vdms_app_path, "/vdms/app/path")
        self.assertEqual(result.config_files_for_vdms, ["config1.json", "config2.json"])

        # Check that the mocked methods were called with the expected arguments
        mock_get_valid_test_name.assert_called_once_with(
            testing_args, DEFAULT_NON_REMOTE_PYTHON_TEST_FILTER
        )
        mock_get_valid_vdms_values.assert_called_once_with(
            testing_args, DEFAULT_NON_REMOTE_PYTHON_CONFIG_FILES
        )

    @patch.object(NonRemotePythonTest, "get_valid_vdms_values")
    @patch.object(NonRemotePythonTest, "get_valid_test_name")
    def test_NonRemotePython_fill_default_arguments_exception_handling(
        self, mock_get_valid_test_name, mock_get_valid_vdms_values
    ):
        # Mock get_valid_test_name to raise an exception
        non_remote_python_test = NonRemotePythonTest()
        testing_args = TestingArgs()

        mock_get_valid_test_name.side_effect = Exception("Test name validation failed")

        # Call the method under test and verify that an exception is raised
        with self.assertRaises(Exception) as context:
            non_remote_python_test.fill_default_arguments(testing_args)

        # Check the exception message
        self.assertIn(
            "fill_default_arguments() in NonRemotePythonTest() error: Test name validation failed",
            str(context.exception),
        )

        # Check that the mocked method that raises the exception was called
        mock_get_valid_test_name.assert_called_once_with(
            testing_args, DEFAULT_NON_REMOTE_PYTHON_TEST_FILTER
        )

        # The following method should not be called since an exception is raised before it is reached
        mock_get_valid_vdms_values.assert_not_called()

    #### Tests for run() ####
    @patch("run_all_tests.NonRemotePythonTest.set_values_for_python_client")
    @patch(
        "run_all_tests.NonRemotePythonTest.open_log_files",
        return_value=(MagicMock(), MagicMock()),
    )
    @patch("run_all_tests.NonRemotePythonTest.run_prep_certs_script")
    @patch("run_all_tests.NonRemotePythonTest.run_vdms_server")
    @patch("run_all_tests.NonRemotePythonTest.run_python_tests")
    @patch("run_all_tests.print")
    def test_run_success(
        self,
        mock_print,
        mock_run_python_tests,
        mock_run_vdms_server,
        mock_run_prep_certs_script,
        mock_open_log_files,
        mock_set_values_for_python_client,
    ):
        non_remote_python_test = NonRemotePythonTest()
        testing_args = TestingArgs()
        testing_args.tmp_tests_dir = "/tmp/tests"
        testing_args.stderr_filename = "stderr.log"
        testing_args.stdout_filename = "stdout.log"
        testing_args.tls_stderr_filename = "tls_stderr.log"
        testing_args.tls_stdout_filename = "tls_stdout.log"
        testing_args.vdms_stderr_filename = "vdms_stderr.log"
        testing_args.vdms_stdout_filename = "vdms_stdout.log"
        testing_args.run = True

        # Call the method
        non_remote_python_test.run(testing_args)

        # Check if the necessary methods were called
        mock_set_values_for_python_client.assert_called_once()
        mock_open_log_files.assert_called()
        mock_run_prep_certs_script.assert_called()
        mock_run_vdms_server.assert_called()
        mock_run_python_tests.assert_called()

        # Check if the print statement was made
        mock_print.assert_called_with("Finished")

    @patch(
        "run_all_tests.NonRemotePythonTest.set_values_for_python_client",
        side_effect=Exception("Client setup failed"),
    )
    def test_run_exception(self, mock_set_values_for_python_client):
        non_remote_python_test = NonRemotePythonTest()
        testing_args = TestingArgs()
        testing_args.tmp_tests_dir = "/tmp/tests"
        testing_args.stderr_filename = "stderr.log"
        testing_args.stdout_filename = "stdout.log"
        testing_args.tls_stderr_filename = "tls_stderr.log"
        testing_args.tls_stdout_filename = "tls_stdout.log"
        testing_args.vdms_stderr_filename = "vdms_stderr.log"
        testing_args.vdms_stdout_filename = "vdms_stdout.log"
        testing_args.run = True

        # Call the method and expect an exception
        with self.assertRaises(Exception) as context:
            non_remote_python_test.run(testing_args)

        # Check the exception message
        expected_message = "run() Exception in NonRemotePythonTest:Client setup failed"
        self.assertEqual(str(context.exception), expected_message)


#### Tests for the RemoteTest class
class TestRemoteTest(unittest.TestCase):
    #### Tests for fill_default_arguments() ####
    @patch.object(RemoteTest, "get_valid_google_test_values")
    @patch.object(RemoteTest, "get_valid_minio_values")
    @patch.object(RemoteTest, "get_valid_test_name")
    def test_RemoteTest_fill_default_arguments(
        self,
        mock_get_valid_test_name,
        mock_get_valid_minio_values,
        mock_get_valid_google_test_values,
    ):
        # Set up the mocks with return values
        remote_test = RemoteTest()
        testing_args = TestingArgs()

        mock_get_valid_test_name.return_value = "provided_test_name"
        mock_get_valid_minio_values.return_value = testing_args
        mock_get_valid_google_test_values.return_value = testing_args

        # Call the method under test
        result = remote_test.fill_default_arguments(testing_args)

        # Assertions to check if the default values are set correctly
        self.assertEqual(result.test_name, "provided_test_name")

        # Check that the mocked methods were called with the expected arguments
        mock_get_valid_test_name.assert_called_once_with(
            testing_args, DEFAULT_REMOTE_UNIT_TEST_FILTER
        )
        mock_get_valid_minio_values.assert_called_once_with(testing_args)
        mock_get_valid_google_test_values.assert_called_once_with(testing_args)

    @patch.object(RemoteTest, "get_valid_google_test_values")
    @patch.object(RemoteTest, "get_valid_minio_values")
    @patch.object(RemoteTest, "get_valid_test_name")
    def test_RemoteTest_fill_default_arguments_exception_handling(
        self,
        mock_get_valid_test_name,
        mock_get_valid_minio_values,
        mock_get_valid_google_test_values,
    ):
        # Mock get_valid_test_name to raise an exception
        remote_test = RemoteTest()
        testing_args = TestingArgs()

        mock_get_valid_test_name.side_effect = Exception("Test name validation failed")

        # Call the method under test and verify that an exception is raised
        with self.assertRaises(Exception) as context:
            remote_test.fill_default_arguments(testing_args)

        # Check the exception message
        self.assertIn(
            "fill_default_arguments() in RemoteTest() error: Test name validation failed",
            str(context.exception),
        )

        # Check that the mocked method that raises the exception was called
        mock_get_valid_test_name.assert_called_once_with(
            testing_args, DEFAULT_REMOTE_UNIT_TEST_FILTER
        )

        # The following methods should not be called since an exception is raised before they are reached
        mock_get_valid_minio_values.assert_not_called()
        mock_get_valid_google_test_values.assert_not_called()

    #### Tests for validate_arguments ####
    @patch.object(RemoteTest, "validate_google_test_path")
    @patch.object(RemoteTest, "validate_minio_values")
    def test_RemoteTest_validate_arguments(
        self, mock_validate_minio_values, mock_validate_google_test_path
    ):
        # Call the method under test
        remote_test = RemoteTest()
        testing_args = TestingArgs()
        parser = argparse.ArgumentParser()

        remote_test.validate_arguments(testing_args, parser)

        # Check that the validation methods were called with the correct arguments
        mock_validate_minio_values.assert_called_once_with(testing_args, parser)
        mock_validate_google_test_path.assert_called_once_with(testing_args, parser)

    def test_RemoteTest_validate_arguments_with_none_parser(self):
        # Call the method under test with a None parser and verify that an exception is raised
        remote_test = RemoteTest()
        testing_args = TestingArgs()
        parser = argparse.ArgumentParser()

        with self.assertRaises(Exception) as context:
            remote_test.validate_arguments(testing_args, None)

        # Check the exception message
        self.assertIn(
            "validate_arguments() in RemoteTest error: parser is None",
            str(context.exception),
        )

    @patch.object(RemoteTest, "validate_google_test_path")
    @patch.object(RemoteTest, "validate_minio_values")
    def test_RemoteTest_validate_arguments_minio_values_exception(
        self, mock_validate_minio_values, mock_validate_google_test_path
    ):
        # Mock validate_minio_values to raise an exception
        remote_test = RemoteTest()
        testing_args = TestingArgs()
        parser = argparse.ArgumentParser()

        mock_validate_minio_values.side_effect = Exception("MinIO validation failed")

        # Call the method under test and verify that an exception is raised
        with self.assertRaises(Exception) as context:
            remote_test.validate_arguments(testing_args, parser)

        # Check the exception message
        self.assertIn(
            "validate_arguments() in RemoteTest error: MinIO validation failed",
            str(context.exception),
        )

    @patch.object(RemoteTest, "validate_google_test_path")
    @patch.object(RemoteTest, "validate_minio_values")
    def test_RemoteTest_validate_arguments_google_test_path_exception(
        self, mock_validate_minio_values, mock_validate_google_test_path
    ):
        # Mock validate_google_test_path to raise an exception
        remote_test = RemoteTest()
        testing_args = TestingArgs()
        parser = argparse.ArgumentParser()

        mock_validate_google_test_path.side_effect = Exception(
            "GoogleTestPath validation failed"
        )

        # Call the method under test and verify that an exception is raised
        with self.assertRaises(Exception) as context:
            remote_test.validate_arguments(testing_args, parser)

        # Check the exception message
        self.assertIn(
            "validate_arguments() in RemoteTest error: GoogleTestPath validation failed",
            str(context.exception),
        )

    #### Tests for Run() ####
    @patch(
        "run_all_tests.RemoteTest.open_log_files",
        return_value=(MagicMock(), MagicMock()),
    )
    @patch("run_all_tests.RemoteTest.run_minio_server")
    @patch("run_all_tests.RemoteTest.run_google_tests")
    @patch("run_all_tests.print")
    def test_run_success(
        self,
        mock_print,
        mock_run_google_tests,
        mock_run_minio_server,
        mock_open_log_files,
    ):
        remote_test = RemoteTest()
        testing_args = TestingArgs()
        testing_args.tmp_tests_dir = "/tmp/tests"
        testing_args.stderr_filename = "stderr.log"
        testing_args.stdout_filename = "stdout.log"
        testing_args.minio_stderr_filename = "minio_stderr.log"
        testing_args.minio_stdout_filename = "minio_stdout.log"
        testing_args.run = True

        # Call the method
        remote_test.run(testing_args)

        # Check if the necessary methods were called
        mock_open_log_files.assert_called()
        mock_run_minio_server.assert_called()
        mock_run_google_tests.assert_called()

        # Check if the print statement was made
        mock_print.assert_called_with("Finished")

    @patch(
        "run_all_tests.RemoteTest.open_log_files",
        side_effect=Exception("Log file open failed"),
    )
    def test_run_exception(self, mock_open_log_files):
        remote_test = RemoteTest()
        testing_args = TestingArgs()
        testing_args.tmp_tests_dir = "/tmp/tests"
        testing_args.stderr_filename = "stderr.log"
        testing_args.stdout_filename = "stdout.log"
        testing_args.minio_stderr_filename = "minio_stderr.log"
        testing_args.minio_stdout_filename = "minio_stdout.log"
        testing_args.run = True

        # Call the method and expect an exception
        with self.assertRaises(Exception) as context:
            remote_test.run(testing_args)

        # Check the exception message
        expected_message = "run() Exception in RemoteTest:Log file open failed"
        self.assertEqual(str(context.exception), expected_message)


#### Tests for the RemotePythonTest class
class TestRemotePythonTest(unittest.TestCase):
    #### Tests for set_env_var_for_skipping_python_tests ####
    @patch("os.environ")
    @patch("builtins.print")
    def test_set_env_var_for_skipping_python_tests(self, mock_print, mock_environ):
        # Call the method under test

        run_all_tests.DEBUG_MODE = True

        remote_python_test = RemotePythonTest()

        remote_python_test.set_env_var_for_skipping_python_tests()

        # Check that the environment variable was set correctly
        mock_environ.__setitem__.assert_called_once_with(
            "VDMS_SKIP_REMOTE_PYTHON_TESTS", "True"
        )

        # Check that the print statement was called with the correct message
        mock_print.assert_called_once_with(
            "Setting to True the VDMS_SKIP_REMOTE_PYTHON_TESTS env var"
        )

    #### Tests for fill_default_arguments ####
    @patch.object(RemotePythonTest, "get_valid_vdms_values")
    @patch.object(RemotePythonTest, "get_valid_minio_values")
    @patch.object(RemotePythonTest, "get_valid_test_name")
    def test_RemotePythonTest_fill_default_arguments(
        self,
        mock_get_valid_test_name,
        mock_get_valid_minio_values,
        mock_get_valid_vdms_values,
    ):
        # Set up the mocks with return values
        remote_python_test = RemotePythonTest()
        testing_args = TestingArgs()
        run_all_tests.DEBUG_MODE = True

        mock_get_valid_test_name.return_value = "provided_test_name"
        mock_get_valid_minio_values.return_value = testing_args
        mock_get_valid_vdms_values.return_value = (
            "/vdms/app/path",
            DEFAULT_REMOTE_PYTHON_CONFIG_FILES,
        )

        # Call the method under test
        result = remote_python_test.fill_default_arguments(testing_args)

        # Assertions to check if the default values are set correctly
        self.assertEqual(result.test_name, "provided_test_name")
        self.assertEqual(result.vdms_app_path, "/vdms/app/path")
        self.assertEqual(
            result.config_files_for_vdms, DEFAULT_REMOTE_PYTHON_CONFIG_FILES
        )

        # Check that the mocked methods were called with the expected arguments
        mock_get_valid_test_name.assert_called_once_with(
            testing_args, DEFAULT_REMOTE_PYTHON_TEST_FILTER
        )
        mock_get_valid_minio_values.assert_called_once_with(testing_args)
        mock_get_valid_vdms_values.assert_called_once_with(
            testing_args, DEFAULT_REMOTE_PYTHON_CONFIG_FILES
        )

    @patch.object(RemotePythonTest, "get_valid_vdms_values")
    @patch.object(RemotePythonTest, "get_valid_minio_values")
    @patch.object(RemotePythonTest, "get_valid_test_name")
    def test_RemotePythonTest_fill_default_arguments_exception_handling(
        self,
        mock_get_valid_test_name,
        mock_get_valid_minio_values,
        mock_get_valid_vdms_values,
    ):
        # Mock get_valid_test_name to raise an exception
        remote_python_test = RemotePythonTest()
        testing_args = TestingArgs()

        mock_get_valid_test_name.side_effect = Exception("Test name validation failed")

        # Call the method under test and verify that an exception is raised
        with self.assertRaises(Exception) as context:
            remote_python_test.fill_default_arguments(testing_args)

        # Check the exception message
        self.assertIn(
            "fill_default_arguments() in RemotePythonTest() error: Test name validation failed",
            str(context.exception),
        )

        # Check that the mocked method that raises the exception was called
        mock_get_valid_test_name.assert_called_once_with(
            testing_args, DEFAULT_REMOTE_PYTHON_TEST_FILTER
        )

        # The following methods should not be called since an exception is raised before they are reached
        mock_get_valid_minio_values.assert_not_called()
        mock_get_valid_vdms_values.assert_not_called()

    #### Tests for validate_arguments ####
    @patch.object(RemotePythonTest, "validate_google_test_path")
    @patch.object(RemotePythonTest, "validate_vdms_values")
    @patch.object(RemotePythonTest, "validate_minio_values")
    def test_RemotePythonTest_validate_arguments(
        self,
        mock_validate_minio_values,
        mock_validate_vmds_values,
        mock_validate_google_test_path,
    ):
        # Call the method under test
        remote_python_test = RemotePythonTest()
        testing_args = TestingArgs()
        parser = argparse.ArgumentParser()

        remote_python_test.validate_arguments(testing_args, parser)

        # Check that the validation methods were called with the correct arguments
        mock_validate_minio_values.assert_called_once_with(testing_args, parser)
        mock_validate_vmds_values.assert_called_once_with(testing_args, parser)
        mock_validate_google_test_path.assert_called_once_with(testing_args, parser)

    def test_RemotePythonTest_validate_arguments_with_none_parser(self):
        # Call the method under test with a None parser and verify that an exception is raised
        remote_python_test = RemotePythonTest()
        testing_args = TestingArgs()

        with self.assertRaises(Exception) as context:
            remote_python_test.validate_arguments(testing_args, None)

        # Check the exception message
        self.assertIn(
            "validate_arguments() in RemotePythonTest error: parser is None",
            str(context.exception),
        )

    @patch.object(RemotePythonTest, "validate_google_test_path")
    @patch.object(RemotePythonTest, "validate_vdms_values")
    @patch.object(RemotePythonTest, "validate_minio_values")
    def test_RemotePythonTest_validate_arguments_exception_handling(
        self,
        mock_validate_minio_values,
        mock_validate_vmds_values,
        mock_validate_google_test_path,
    ):
        # Mock validate_minio_values to raise an exception

        remote_python_test = RemotePythonTest()
        testing_args = TestingArgs()
        parser = argparse.ArgumentParser()

        mock_validate_minio_values.side_effect = Exception("MinIO validation failed")

        # Call the method under test and verify that an exception is raised
        with self.assertRaises(Exception) as context:
            remote_python_test.validate_arguments(testing_args, parser)

        # Check the exception message
        self.assertIn(
            "validate_arguments() in RemotePythonTest error: MinIO validation failed",
            str(context.exception),
        )

        # Check that the mocked method that raises the exception was called
        mock_validate_minio_values.assert_called_once_with(testing_args, parser)

        # The following methods should not be called since an exception is raised before they are reached
        mock_validate_vmds_values.assert_not_called()
        mock_validate_google_test_path.assert_not_called()

    #### Tests for run ####
    @patch("run_all_tests.RemotePythonTest.set_values_for_python_client")
    @patch(
        "run_all_tests.RemotePythonTest.open_log_files",
        return_value=(MagicMock(), MagicMock()),
    )
    @patch("run_all_tests.RemotePythonTest.run_prep_certs_script")
    @patch("run_all_tests.RemotePythonTest.run_vdms_server")
    @patch("run_all_tests.RemotePythonTest.run_minio_server")
    @patch("run_all_tests.RemotePythonTest.set_env_var_for_skipping_python_tests")
    @patch("run_all_tests.RemotePythonTest.run_python_tests")
    @patch("run_all_tests.print")
    def test_run_success(
        self,
        mock_print,
        mock_run_python_tests,
        mock_set_env_var_for_skipping_python_tests,
        mock_run_minio_server,
        mock_run_vdms_server,
        mock_run_prep_certs_script,
        mock_open_log_files,
        mock_set_values_for_python_client,
    ):
        remote_python_test = RemotePythonTest()
        testing_args = TestingArgs()
        testing_args.tmp_tests_dir = "/tmp/tests"
        testing_args.stderr_filename = "stderr.log"
        testing_args.stdout_filename = "stdout.log"
        testing_args.tls_stderr_filename = "tls_stderr.log"
        testing_args.tls_stdout_filename = "tls_stdout.log"
        testing_args.vdms_stderr_filename = "vdms_stderr.log"
        testing_args.vdms_stdout_filename = "vdms_stdout.log"
        testing_args.minio_stderr_filename = "minio_stderr.log"
        testing_args.minio_stdout_filename = "minio_stdout.log"
        testing_args.run = True

        # Call the method
        remote_python_test.run(testing_args)

        # Check if the necessary methods were called
        mock_set_values_for_python_client.assert_called_once()
        mock_open_log_files.assert_called()
        mock_run_prep_certs_script.assert_called()
        mock_run_vdms_server.assert_called()
        mock_run_minio_server.assert_called()
        mock_set_env_var_for_skipping_python_tests.assert_called_once()
        mock_run_python_tests.assert_called()

        # Check if the print statement was made
        mock_print.assert_called_with("Finished")

    @patch(
        "run_all_tests.RemotePythonTest.set_values_for_python_client",
        side_effect=Exception("Client setup failed"),
    )
    def test_run_exception(self, mock_set_values_for_python_client):
        remote_python_test = RemotePythonTest()
        testing_args = TestingArgs()
        testing_args.tmp_tests_dir = "/tmp/tests"
        testing_args.stderr_filename = "stderr.log"
        testing_args.stdout_filename = "stdout.log"
        testing_args.tls_stderr_filename = "tls_stderr.log"
        testing_args.tls_stdout_filename = "tls_stdout.log"
        testing_args.vdms_stderr_filename = "vdms_stderr.log"
        testing_args.vdms_stdout_filename = "vdms_stdout.log"
        testing_args.minio_stderr_filename = "minio_stderr.log"
        testing_args.minio_stdout_filename = "minio_stdout.log"
        testing_args.run = True

        # Call the method and expect an exception
        with self.assertRaises(Exception) as context:
            remote_python_test.run(testing_args)

        # Check the exception message
        expected_message = "run() Exception in RemotePythonTest: Client setup failed"
        self.assertEqual(str(context.exception), expected_message)


#### Tests for the TestingParser class
class TestTestingParser(unittest.TestCase):
    #### Tests for parse_arguments ####
    @patch("argparse.ArgumentParser.parse_args")
    def test_parse_arguments(self, mock_parse_args):

        testing_args = TestingParser()

        # Set up the mock to return a namespace with test arguments
        mock_parse_args.return_value = MagicMock(
            json="path/to/config.json",
            minio_port=9000,
            config_files_for_vdms=["config1.json", "config2.json"],
            tmp_tests_dir="/tmp/tests",
            stderr_filename="stderr.log",
            googletest="path/to/googletest",
            keep_tmp_tests_dir=True,
            minio_app_path="path/to/minio",
            test_name="TestName",
            stdout_filename="stdout.log",
            minio_password="",
            stop_tests_on_failure=False,
            type_of_test="ut",
            minio_username="minio_user",
            vdms_app_path="path/to/vdms",
            minio_console_port=9001,
            neo4j_port=7687,
            neo4j_password="",
            neo4j_username="neo4j_user",
            run=True,
        )

        # Call the method under test
        args = testing_args.parse_arguments()

        # Assertions to check if the arguments are parsed correctly
        self.assertEqual(args.json, "path/to/config.json")
        self.assertEqual(args.minio_port, 9000)
        self.assertEqual(args.config_files_for_vdms, ["config1.json", "config2.json"])
        self.assertEqual(args.tmp_tests_dir, "/tmp/tests")
        self.assertEqual(args.stderr_filename, "stderr.log")
        self.assertEqual(args.googletest, "path/to/googletest")
        self.assertEqual(args.keep_tmp_tests_dir, True)
        self.assertEqual(args.minio_app_path, "path/to/minio")
        self.assertEqual(args.test_name, "TestName")
        self.assertEqual(args.stdout_filename, "stdout.log")
        self.assertEqual(args.minio_password, "")
        self.assertEqual(args.stop_tests_on_failure, False)
        self.assertEqual(args.type_of_test, "ut")
        self.assertEqual(args.minio_username, "minio_user")
        self.assertEqual(args.vdms_app_path, "path/to/vdms")
        self.assertEqual(args.minio_console_port, 9001)
        self.assertEqual(args.neo4j_port, 7687)
        self.assertEqual(args.neo4j_password, "")
        self.assertEqual(args.neo4j_username, "neo4j_user")
        self.assertEqual(args.run, True)

        # Check that the parse_args method was called
        mock_parse_args.assert_called_once()

    def test_parse_arguments_exception(self):
        testing_parser = TestingParser()

        # Simulate an exception during argument parsing
        with patch(
            "argparse.ArgumentParser.parse_args",
            side_effect=Exception("Mocked parse_args exception"),
        ):
            with self.assertRaises(Exception) as context:
                testing_parser.parse_arguments()

            # Assert that the exception message is as expected
            self.assertIn(
                "parse_arguments() error: Mocked parse_args exception",
                str(context.exception),
            )

    #### Tests for get_parser() ####
    def test_get_parser(self):
        testing_args = TestingParser()

        # Set the parser attribute to a MagicMock object
        expected_parser = MagicMock(spec=argparse.ArgumentParser)
        testing_args.parser = expected_parser

        # Call the method under test
        result = testing_args.get_parser()

        # Assertions to check if the get_parser method returns the parser attribute
        self.assertEqual(result, expected_parser)

    #### Tests for set_parser() ####
    def test_set_parser(self):
        testing_args = TestingParser()

        # Create a MagicMock object to represent a new parser
        new_parser = MagicMock(spec=argparse.ArgumentParser)

        # Call the method under test with the new parser
        testing_args.set_parser(new_parser)

        # Assertions to check if the set_parser method sets the parser attribute correctly
        self.assertIs(testing_args.parser, new_parser)

    #### Tests for read_json_config_file ####
    @patch("os.path.exists")
    @patch("json.loads")
    @patch(
        "builtins.open", new_callable=mock_open, read_data='{"test_name": "TestName"}'
    )
    def test_read_json_config_file(self, mock_file, mock_json_loads, mock_path_exists):
        testing_args = TestingParser()
        parser = MagicMock(spec=argparse.ArgumentParser)

        # Set up the mocks
        mock_path_exists.return_value = True
        mock_json_loads.return_value = {"test_name": "TestName"}

        # Call the method under test
        result = testing_args.read_json_config_file("path/to/config.json", parser)

        # Assertions to check if the JSON config file is read and converted correctly
        self.assertIsInstance(result, TestingArgs)
        self.assertEqual(result.test_name, "TestName")

        # Check that the file was opened, and json.loads was called with the correct data
        mock_file.assert_called_once_with("path/to/config.json", "r")
        mock_json_loads.assert_called_once_with('{"test_name": "TestName"}')

    @patch("os.path.exists")
    @patch(
        "builtins.open", new_callable=mock_open, read_data='{"test_name": "TestName"}'
    )
    def test_read_json_config_file_not_exist(self, mock_file, mock_path_exists):
        testing_parser = TestingParser()
        parser = MagicMock(spec=argparse.ArgumentParser)

        # Set up the mock to return False, indicating the file does not exist
        mock_path_exists.return_value = False
        parser.error = MagicMock()

        # Call the method under test
        testing_parser.read_json_config_file("path/to/nonexistent.json", parser)

        # Check that parser.error was called with the correct message
        parser.error.assert_called_once_with(
            "path/to/nonexistent.json does not exist or there is not access to it"
        )

    @patch("os.path.exists")
    @patch("builtins.open", new_callable=mock_open, read_data="invalid json")
    def test_read_json_config_file_invalid_json(self, mock_file, mock_path_exists):
        testing_parser = TestingParser()
        parser = MagicMock(spec=argparse.ArgumentParser)

        # Set up the mock to return True, indicating the file exists
        mock_path_exists.return_value = True
        parser.error = MagicMock()

        # Simulate json.loads raising a JSONDecodeError
        with patch(
            "json.loads",
            side_effect=JSONDecodeError("Expecting value", "invalid json", 0),
        ):
            # Call the method under test and expect it to raise an exception
            with self.assertRaises(Exception) as context:
                testing_parser.read_json_config_file("path/to/invalid.json", parser)

            # Check the exception message
            self.assertIn("read_json_config_file error:", str(context.exception))

            # Since we are expecting a new exception to be raised, not calling parser.error
            parser.error.assert_not_called()

    def test_read_json_config_file_parser_none(self):
        # Test that an exception is raised when parser is None

        testing_parser = TestingParser()
        parser = None

        with self.assertRaises(Exception) as context:
            testing_parser.read_json_config_file("path/to/invalid.json", parser)
        self.assertIn("parser is None", str(context.exception))

    #### Tests for convert_json_to_testing_args ####
    def test_convert_json_to_testing_args(self):
        testing_parser = TestingParser()
        parser = MagicMock(spec=argparse.ArgumentParser)

        # Create a JSON dictionary with test data
        json_config_data = {
            "vdms_app_path": "/path/to/vdms",
            "googletest_path": "/path/to/googletest",
            "test_name": "TestName",
            "minio_username": "minio_user",
            "minio_password": "minio_pass",
            "type_of_test": "ut",
            "tmp_tests_dir": "/tmp/tests",
            "stderr_filename": "stderr.log",
            "stdout_filename": "stdout.log",
            "config_files_for_vdms": ["config1.json", "config2.json"],
            "minio_app_path": "/path/to/minio",
            "minio_port": 9000,
            "stop_tests_on_failure": True,
            "keep_tmp_tests_dir": False,
            "minio_console_port": 9001,
            "neo4j_port": 7687,
            "neo4j_password": "neo4j123",
            "neo4j_username": "neo4j_user",
            "neo4j_endpoint": "neo4j_endpoint",
            "run": True,
        }

        # Call the method under test
        result = testing_parser.convert_json_to_testing_args(json_config_data, parser)

        # Assertions to check if the JSON data is converted correctly
        self.assertIsInstance(result, TestingArgs)
        self.assertEqual(result.vdms_app_path, "/path/to/vdms")
        self.assertEqual(result.googletest_path, "/path/to/googletest")
        self.assertEqual(result.test_name, "TestName")
        self.assertEqual(result.minio_username, "minio_user")
        self.assertEqual(result.minio_password, "minio_pass")
        self.assertEqual(result.type_of_test, "ut")
        self.assertEqual(result.tmp_tests_dir, "/tmp/tests")
        self.assertEqual(result.stderr_filename, "stderr.log")
        self.assertEqual(result.stdout_filename, "stdout.log")
        self.assertEqual(result.config_files_for_vdms, ["config1.json", "config2.json"])
        self.assertEqual(result.minio_app_path, "/path/to/minio")
        self.assertEqual(result.minio_port, 9000)
        self.assertEqual(result.stop_tests_on_failure, True)
        self.assertEqual(result.keep_tmp_tests_dir, False)
        self.assertEqual(result.minio_console_port, 9001)
        self.assertEqual(result.neo4j_port, 7687)
        self.assertEqual(result.neo4j_password, "neo4j123")
        self.assertEqual(result.neo4j_username, "neo4j_user")
        self.assertEqual(result.neo4j_endpoint, "neo4j_endpoint")
        self.assertEqual(result.run, True)

    def _test_invalid_data_in_convert_json_to_testing_args(
        self, json_config_data, attribute, datatype
    ):
        testing_parser = TestingParser()
        parser = MagicMock(spec=argparse.ArgumentParser)

        # Mock the parser.error method to raise an exception
        parser.error = MagicMock(
            side_effect=Exception(
                f"convert_json_to_testing_args error: '{attribute}' value in the JSON file is not a valid {datatype}"
            )
        )

        # Call the method under test and expect it to raise an exception
        with self.assertRaises(Exception) as context:
            testing_parser.convert_json_to_testing_args(json_config_data, parser)

        # Check that parser.error was called with the correct message
        parser.error.assert_called_once_with(
            f"convert_json_to_testing_args error: '{attribute}' value in the JSON file is not a valid {datatype}"
        )

        # Check that an exception was raised
        self.assertEqual(
            str(context.exception),
            f"convert_json_to_testing_args error: '{attribute}' value in the JSON file is not a valid {datatype}",
        )

    def test_convert_json_to_testing_args_invalid_vdms_app_path_data(self):

        # Create a JSON dictionary with invalid data (e.g., wrong data type)
        json_config_data = {
            "vdms_app_path": 123,  # Invalid data type, expecting a string
            # ... (other key-value pairs with invalid data types)
        }

        self._test_invalid_data_in_convert_json_to_testing_args(
            json_config_data, "vdms_app_path", "string"
        )

    def test_convert_json_to_testing_args_invalid_googletest_path_data(self):
        # Create a JSON dictionary with invalid data (e.g., wrong data type)
        json_config_data = {
            "googletest_path": 123,  # Invalid data type, expecting a string
            # ... (other key-value pairs with invalid data types)
        }

        self._test_invalid_data_in_convert_json_to_testing_args(
            json_config_data, "googletest_path", "string"
        )

    def test_convert_json_to_testing_args_invalid_test_name_data(self):
        # Create a JSON dictionary with invalid data (e.g., wrong data type)
        json_config_data = {
            "test_name": 123,  # Invalid data type, expecting a string
            # ... (other key-value pairs with invalid data types)
        }

        self._test_invalid_data_in_convert_json_to_testing_args(
            json_config_data, "test_name", "string"
        )

    def test_convert_json_to_testing_args_invalid_minio_username_data(self):
        # Create a JSON dictionary with invalid data (e.g., wrong data type)
        json_config_data = {
            "minio_username": 123,  # Invalid data type, expecting a string
            # ... (other key-value pairs with invalid data types)
        }

        self._test_invalid_data_in_convert_json_to_testing_args(
            json_config_data, "minio_username", "string"
        )

    def test_convert_json_to_testing_args_invalid_minio_password_data(self):
        # Create a JSON dictionary with invalid data (e.g., wrong data type)
        json_config_data = {
            "minio_password": 123,  # Invalid data type, expecting a string
            # ... (other key-value pairs with invalid data types)
        }

        self._test_invalid_data_in_convert_json_to_testing_args(
            json_config_data, "minio_password", "string"
        )

    def test_convert_json_to_testing_args_invalid_type_of_test_data(self):
        # Create a JSON dictionary with invalid data (e.g., wrong data type)
        json_config_data = {
            "type_of_test": 123,  # Invalid data type, expecting a string
            # ... (other key-value pairs with invalid data types)
        }

        self._test_invalid_data_in_convert_json_to_testing_args(
            json_config_data, "type_of_test", "string"
        )

    def test_convert_json_to_testing_args_invalid_tmp_tests_dir_data(self):
        # Create a JSON dictionary with invalid data (e.g., wrong data type)
        json_config_data = {
            "tmp_tests_dir": 123,  # Invalid data type, expecting a string
            # ... (other key-value pairs with invalid data types)
        }

        self._test_invalid_data_in_convert_json_to_testing_args(
            json_config_data, "tmp_tests_dir", "string"
        )

    def test_convert_json_to_testing_args_invalid_stderr_filename_data(self):
        # Create a JSON dictionary with invalid data (e.g., wrong data type)
        json_config_data = {
            "stderr_filename": 123,  # Invalid data type, expecting a string
            # ... (other key-value pairs with invalid data types)
        }

        self._test_invalid_data_in_convert_json_to_testing_args(
            json_config_data, "stderr_filename", "string"
        )

    def test_convert_json_to_testing_args_invalid_stdout_filename_data(self):
        # Create a JSON dictionary with invalid data (e.g., wrong data type)
        json_config_data = {
            "stdout_filename": 123,  # Invalid data type, expecting a string
            # ... (other key-value pairs with invalid data types)
        }

        self._test_invalid_data_in_convert_json_to_testing_args(
            json_config_data, "stdout_filename", "string"
        )

    def test_convert_json_to_testing_args_invalid_config_files_for_vdms_data(self):
        # Create a JSON dictionary with invalid data (e.g., wrong data type)
        json_config_data = {
            "config_files_for_vdms": 123,  # Invalid data type, expecting a string
            # ... (other key-value pairs with invalid data types)
        }

        self._test_invalid_data_in_convert_json_to_testing_args(
            json_config_data, "config_files_for_vdms", "list"
        )

    def test_convert_json_to_testing_args_invalid_minio_app_path_data(self):
        # Create a JSON dictionary with invalid data (e.g., wrong data type)
        json_config_data = {
            "minio_app_path": 123,  # Invalid data type, expecting a string
            # ... (other key-value pairs with invalid data types)
        }

        self._test_invalid_data_in_convert_json_to_testing_args(
            json_config_data, "minio_app_path", "string"
        )

    def test_convert_json_to_testing_args_invalid_keep_tmp_tests_dir_data(self):
        # Create a JSON dictionary with invalid data (e.g., wrong data type)
        json_config_data = {
            "keep_tmp_tests_dir": 123,  # Invalid data type, expecting a string
            # ... (other key-value pairs with invalid data types)
        }

        self._test_invalid_data_in_convert_json_to_testing_args(
            json_config_data, "keep_tmp_tests_dir", "bool"
        )

    def test_convert_json_to_testing_args_invalid_stop_tests_on_failure_data(self):
        # Create a JSON dictionary with invalid data (e.g., wrong data type)
        json_config_data = {
            "stop_tests_on_failure": 123  # Invalid data type, expecting a string
        }

        self._test_invalid_data_in_convert_json_to_testing_args(
            json_config_data, "stop_tests_on_failure", "bool"
        )

    def test_convert_json_to_testing_args_minio_port_data(self):
        # Create a JSON dictionary with invalid data (e.g., wrong data type)
        json_config_data = {
            "minio_port": "test",  # Invalid data type, expecting a string
            # ... (other key-value pairs with invalid data types)
        }

        self._test_invalid_data_in_convert_json_to_testing_args(
            json_config_data, "minio_port", "integer"
        )

    def test_convert_json_to_testing_args_minio_console_port_data(self):
        # Create a JSON dictionary with invalid data (e.g., wrong data type)
        json_config_data = {
            "minio_console_port": "test",  # Invalid data type, expecting a string
            # ... (other key-value pairs with invalid data types)
        }

        self._test_invalid_data_in_convert_json_to_testing_args(
            json_config_data, "minio_console_port", "integer"
        )

    def test_convert_json_to_testing_args_neo4j_port_data(self):
        # Create a JSON dictionary with invalid data (e.g., wrong data type)
        json_config_data = {
            "neo4j_port": "test",  # Invalid data type, expecting a string
            # ... (other key-value pairs with invalid data types)
        }

        self._test_invalid_data_in_convert_json_to_testing_args(
            json_config_data, "neo4j_port", "integer"
        )

    def test_convert_json_to_testing_args_invalid_neo4j_password_data(self):
        # Create a JSON dictionary with invalid data (e.g., wrong data type)
        json_config_data = {
            "neo4j_password": 123,  # Invalid data type, expecting a string
            # ... (other key-value pairs with invalid data types)
        }

        self._test_invalid_data_in_convert_json_to_testing_args(
            json_config_data, "neo4j_password", "string"
        )

    def test_convert_json_to_testing_args_invalid_neo4j_username_data(self):
        # Create a JSON dictionary with invalid data (e.g., wrong data type)
        json_config_data = {
            "neo4j_username": 123,  # Invalid data type, expecting a string
            # ... (other key-value pairs with invalid data types)
        }

        self._test_invalid_data_in_convert_json_to_testing_args(
            json_config_data, "neo4j_username", "string"
        )

    def test_convert_json_to_testing_args_invalid_neo4j_endpoint_data(self):
        # Create a JSON dictionary with invalid data (e.g., wrong data type)
        json_config_data = {
            "neo4j_endpoint": 123,  # Invalid data type, expecting a string
            # ... (other key-value pairs with invalid data types)
        }

        self._test_invalid_data_in_convert_json_to_testing_args(
            json_config_data, "neo4j_endpoint", "string"
        )

    def test_convert_json_to_testing_args_invalid_run_data(self):
        # Create a JSON dictionary with invalid data (e.g., wrong data type)
        json_config_data = {
            "run": 123,  # Invalid data type, expecting a string
            # ... (other key-value pairs with invalid data types)
        }

        self._test_invalid_data_in_convert_json_to_testing_args(
            json_config_data, "run", "bool"
        )

    def test_convert_json_to_testing_args_parser_none(self):
        # Test that an exception is raised when parser is None

        testing_parser = TestingParser()
        # Create a JSON dictionary with invalid data (e.g., wrong data type)
        json_config_data = {
            "vdms_app_path": 123,  # Invalid data type, expecting a string
            # ... (other key-value pairs with invalid data types)
        }

        parser = None

        # Call the method under test and expect it to raise an exception
        with self.assertRaises(Exception) as context:
            testing_parser.convert_json_to_testing_args(json_config_data, parser)

        # Check that an exception was raised
        self.assertEqual(
            str(context.exception), "convert_json_to_testing_args error: parser is None"
        )

    #### Tests for convert_namespace_to_testing_args ####
    def test_convert_namespace_to_testing_args_valid_data(self):
        testing_parser = TestingParser()
        parser = MagicMock(spec=argparse.ArgumentParser)
        # Create a Namespace object with valid test data
        namespace_data = argparse.Namespace(
            vdms_app_path="/path/to/vdms",
            googletest_path="path/to/googletest",
            test_name="TestName",
            minio_username="minio_user",
            minio_password="",
            type_of_test="ut",
            tmp_tests_dir="/tmp/tests",
            stderr_filename="stderr.log",
            stdout_filename="stdout.log",
            config_files_for_vdms=["config1.json", "config2.json"],
            minio_app_path="/path/to/minio",
            minio_port=9000,
            stop_tests_on_failure=False,
            keep_tmp_tests_dir=True,
            minio_console_port=9001,
            neo4j_port=7687,
            neo4j_password="",
            neo4j_username="neo4j_user",
            neo4j_endpoint="neo4j_endpoint",
            run=True,
        )

        # Call the method under test
        result = testing_parser.convert_namespace_to_testing_args(
            namespace_data, parser
        )

        # Assertions to check if the Namespace data is converted correctly
        self.assertIsInstance(result, TestingArgs)
        self.assertEqual(result.vdms_app_path, "/path/to/vdms")
        # ... (assertions for other attributes)

    def test_convert_namespace_to_testing_args_invalid_config_files_for_vdms_data(self):
        testing_parser = TestingParser()
        parser = MagicMock(spec=argparse.ArgumentParser)

        # Create a Namespace object with invalid data (e.g., wrong data type)
        namespace_data = argparse.Namespace(
            vdms_app_path="/path/to/vdms",
            googletest_path="path/to/googletest",  # Correct attribute name
            test_name="TestName",
            minio_username="minio_user",
            minio_password="",
            type_of_test="ut",
            tmp_tests_dir="/tmp/tests",
            stderr_filename="stderr.log",
            stdout_filename="stdout.log",
            config_files_for_vdms="not a list",  # Invalid data type, expecting a list
            minio_app_path="/path/to/minio",
            minio_port=9000,
            stop_tests_on_failure="not a bool",  # Invalid data type, expecting a bool
            keep_tmp_tests_dir="not a bool",  # Invalid data type, expecting a bool
            minio_console_port=9001,
            neo4j_port=7687,
            neo4j_password="",
            neo4j_username="neo4j_user",
            neo4j_endpoint="neo4j_endpoint",
            run="not a bool",  # Invalid data type, expecting a bool
        )

        # Mock the parser.error method to raise an exception
        parser.error = MagicMock(side_effect=Exception("Invalid data type"))

        # Call the method under test and expect it to raise an exception
        with self.assertRaises(Exception) as context:
            testing_parser.convert_namespace_to_testing_args(namespace_data, parser)

        # Check that parser.error was called with the correct message
        calls = [
            unittest.mock.call(
                "convert_namespace_to_testing_args() error: 'config_files_for_vdms' value in the namespace is not a valid list"
            )
        ]
        parser.error.assert_has_calls(calls, any_order=True)

        # Check that an exception was raised
        self.assertTrue(context.exception)

    def test_convert_namespace_to_testing_args_parser_none(self):
        # Test that an exception is raised when parser is None

        testing_parser = TestingParser()
        namespaceData = None
        parser = None

        # Call the method under test and expect it to raise an exception
        with self.assertRaises(Exception) as context:
            testing_parser.convert_namespace_to_testing_args(namespaceData, parser)

        # Check that an exception was raised
        self.assertEqual(
            str(context.exception),
            "convert_namespace_to_testing_args() error: parser is None",
        )

    def test_convert_namespace_to_testing_args_invalid_stop_tests_on_failure_data(self):
        testing_parser = TestingParser()
        parser = MagicMock(spec=argparse.ArgumentParser)

        # Create a Namespace object with invalid data (e.g., wrong data type)
        namespace_data = argparse.Namespace(
            vdms_app_path="/path/to/vdms",
            googletest_path="path/to/googletest",  # Correct attribute name
            test_name="TestName",
            minio_username="minio_user",
            minio_password="",
            type_of_test="ut",
            tmp_tests_dir="/tmp/tests",
            stderr_filename="stderr.log",
            stdout_filename="stdout.log",
            config_files_for_vdms=[],
            minio_app_path="/path/to/minio",
            minio_port=9000,
            stop_tests_on_failure="not a bool",  # Invalid data type, expecting a bool
            keep_tmp_tests_dir="not a bool",  # Invalid data type, expecting a bool
            minio_console_port=9001,
            neo4j_port=7687,
            neo4j_password="",
            neo4j_username="neo4j_user",
            neo4j_endpoint="neo4j_endpoint",
            run="not a bool",  # Invalid data type, expecting a bool
        )

        # Mock the parser.error method to raise an exception
        parser.error = MagicMock(side_effect=Exception("Invalid data type"))

        # Call the method under test and expect it to raise an exception
        with self.assertRaises(Exception) as context:
            testing_parser.convert_namespace_to_testing_args(namespace_data, parser)

        # Check that parser.error was called with the correct message
        calls = [
            unittest.mock.call(
                "convert_namespace_to_testing_args() error: 'stop_tests_on_failure' value in the namespace is not a valid bool"
            )
        ]
        parser.error.assert_has_calls(calls, any_order=True)

        # Check that an exception was raised
        self.assertTrue(context.exception)

    def test_convert_namespace_to_testing_args_invalid_keep_tmp_tests_dir_data(self):
        testing_parser = TestingParser()
        parser = MagicMock(spec=argparse.ArgumentParser)

        # Create a Namespace object with invalid data (e.g., wrong data type)
        namespace_data = argparse.Namespace(
            vdms_app_path="/path/to/vdms",
            googletest_path="path/to/googletest",  # Correct attribute name
            test_name="TestName",
            minio_username="minio_user",
            minio_password="",
            type_of_test="ut",
            tmp_tests_dir="/tmp/tests",
            stderr_filename="stderr.log",
            stdout_filename="stdout.log",
            config_files_for_vdms=[],
            minio_app_path="/path/to/minio",
            minio_port=9000,
            stop_tests_on_failure=False,  # Invalid data type, expecting a bool
            keep_tmp_tests_dir="not a bool",  # Invalid data type, expecting a bool
            minio_console_port=9001,
            neo4j_port=7687,
            neo4j_password="",
            neo4j_username="neo4j_user",
            neo4j_endpoint="neo4j_endpoint",
            run="not a bool",  # Invalid data type, expecting a bool
        )

        # Mock the parser.error method to raise an exception
        parser.error = MagicMock(side_effect=Exception("Invalid data type"))

        # Call the method under test and expect it to raise an exception
        with self.assertRaises(Exception) as context:
            testing_parser.convert_namespace_to_testing_args(namespace_data, parser)

        # Check that parser.error was called with the correct message
        calls = [
            unittest.mock.call(
                "convert_namespace_to_testing_args() error: 'keep_tmp_tests_dir' value in the namespace is not a valid bool"
            )
        ]
        parser.error.assert_has_calls(calls, any_order=True)

        # Check that an exception was raised
        self.assertTrue(context.exception)

    def test_convert_namespace_to_testing_args_invalid_run_data(self):
        testing_parser = TestingParser()
        parser = MagicMock(spec=argparse.ArgumentParser)

        # Create a Namespace object with invalid data (e.g., wrong data type)
        namespace_data = argparse.Namespace(
            vdms_app_path="/path/to/vdms",
            googletest_path="path/to/googletest",  # Correct attribute name
            test_name="TestName",
            minio_username="minio_user",
            minio_password="",
            type_of_test="ut",
            tmp_tests_dir="/tmp/tests",
            stderr_filename="stderr.log",
            stdout_filename="stdout.log",
            config_files_for_vdms=[],
            minio_app_path="/path/to/minio",
            minio_port=9000,
            stop_tests_on_failure=False,  # Invalid data type, expecting a bool
            keep_tmp_tests_dir=False,  # Invalid data type, expecting a bool
            minio_console_port=9001,
            neo4j_port=7687,
            neo4j_password="",
            neo4j_username="neo4j_user",
            neo4j_endpoint=f"neo4j://neo4j:7687",
            run="not a bool",  # Invalid data type, expecting a bool
        )

        # Mock the parser.error method to raise an exception
        parser.error = MagicMock(side_effect=Exception("Invalid data type"))

        # Call the method under test and expect it to raise an exception
        with self.assertRaises(Exception) as context:
            testing_parser.convert_namespace_to_testing_args(namespace_data, parser)

        # Check that parser.error was called with the correct message
        calls = [
            unittest.mock.call(
                "convert_namespace_to_testing_args() error: 'run' value in the namespace is not a valid bool"
            )
        ]
        parser.error.assert_has_calls(calls, any_order=True)

        # Check that an exception was raised
        self.assertTrue(context.exception)

    #### Tests for validate_type_test_value ####
    def test_validate_type_test_value_valid(self):
        testing_parser = TestingParser()
        parser = MagicMock(spec=argparse.ArgumentParser)
        testing_args = TestingArgs()

        # Set a valid type_of_test
        testing_args.type_of_test = "ut"

        # Call the method under test
        testing_parser.validate_type_test_value(testing_args, parser)

        # parser.error should not be called with valid data
        parser.error.assert_not_called()

    def test_validate_type_test_value_missing(self):
        testing_parser = TestingParser()
        parser = MagicMock(spec=argparse.ArgumentParser)
        testing_args = TestingArgs()

        # Remove the type_of_test attribute to simulate it being missing
        del testing_args.type_of_test

        # Mock the parser.error method to raise an exception
        parser.error = MagicMock(side_effect=Exception("Missing type_of_test"))

        # Call the method under test and expect it to raise an exception
        with self.assertRaises(Exception) as context:
            testing_parser.validate_type_test_value(testing_args, parser)

        # Check that parser.error was called with the correct message
        parser.error.assert_called_once_with(
            "the following argument is required: -t/--type_of_test "
            + str(TYPE_OF_TESTS_AVAILABLE)
        )

        # Check that an exception was raised
        self.assertEqual(str(context.exception), "Missing type_of_test")

    def test_validate_type_test_value_none(self):
        testing_parser = TestingParser()
        parser = MagicMock(spec=argparse.ArgumentParser)
        testing_args = TestingArgs()

        # Set type_of_test to None to simulate it being explicitly unset
        testing_args.type_of_test = None

        # Mock the parser.error method to raise an exception
        parser.error = MagicMock(side_effect=Exception("type_of_test is None"))

        # Call the method under test and expect it to raise an exception
        with self.assertRaises(Exception) as context:
            testing_parser.validate_type_test_value(testing_args, parser)

        # Check that parser.error was called with the correct message
        parser.error.assert_called_once_with(
            "the following argument is required: -t/--type_of_test "
            + str(TYPE_OF_TESTS_AVAILABLE)
        )

        # Check that an exception was raised
        self.assertEqual(str(context.exception), "type_of_test is None")

    def test_validate_type_test_value_empty(self):
        testing_parser = TestingParser()
        parser = MagicMock(spec=argparse.ArgumentParser)
        testing_args = TestingArgs()

        # Set type_of_test to an empty string to simulate it being empty
        testing_args.type_of_test = ""

        # Mock the parser.error method to raise an exception
        parser.error = MagicMock(side_effect=Exception("type_of_test is empty"))

        # Call the method under test and expect it to raise an exception
        with self.assertRaises(Exception) as context:
            testing_parser.validate_type_test_value(testing_args, parser)

        # Check that parser.error was called with the correct message
        parser.error.assert_called_once_with(
            "the following argument is required: -t/--type_of_test "
            + str(TYPE_OF_TESTS_AVAILABLE)
        )

        # Check that an exception was raised
        self.assertEqual(str(context.exception), "type_of_test is empty")

    #### Tests for validate_stop_testing_value ####
    @patch("builtins.print")
    def test_validate_stop_testing_value_correct_usage(self, mock_print):
        testing_parser = TestingParser()
        testing_args = TestingArgs()

        # Set attributes for a valid GoogleTest type and stop_tests_on_failure flag
        testing_args.type_of_test = "ut"
        testing_args.stop_tests_on_failure = True

        # Call the method under test
        testing_parser.validate_stop_testing_value(testing_args)

        # Check that the print function was not called since usage is correct
        mock_print.assert_not_called()

    @patch("builtins.print")
    def test_validate_stop_testing_value_incorrect_usage(self, mock_print):
        testing_parser = TestingParser()
        testing_args = TestingArgs()

        # Set attributes for a non-GoogleTest type and stop_tests_on_failure flag
        testing_args.type_of_test = "non_gt_type"
        testing_args.stop_tests_on_failure = True

        # Call the method under test
        testing_parser.validate_stop_testing_value(testing_args)

        # Check that the print function was called with the warning message
        mock_print.assert_called_once()
        warning_message = "stop_tests_on_failure flag is only used by Googletest tests.\nThis flag will be ignored"
        mock_print.assert_called_with(warning_message)

    def test_validate_stop_testing_value_not_set(self):
        testing_parser = TestingParser()
        testing_args = TestingArgs()

        # Set attributes without the stop_tests_on_failure flag
        testing_args.type_of_test = "ut"
        testing_args.stop_tests_on_failure = None

        # Call the method under test
        testing_parser.validate_stop_testing_value(testing_args)

    #### Tests for validate_common_arguments ####

    def test_validate_common_arguments_with_parser(self):
        testing_parser = TestingParser()
        testing_args = TestingArgs()
        parser = MagicMock(spec=argparse.ArgumentParser)

        # Mock the internal validation methods
        with patch.object(
            testing_parser, "validate_type_test_value"
        ) as mock_validate_type, patch.object(
            testing_parser, "validate_stop_testing_value"
        ) as mock_validate_stop:
            # Call the method under test
            testing_parser.validate_common_arguments(testing_args, parser)

            # Check that the internal validation methods were called
            mock_validate_type.assert_called_once_with(testing_args, parser)
            mock_validate_stop.assert_called_once_with(testing_args)

    def test_validate_common_arguments_with_none_parser(self):
        testing_parser = TestingParser()
        testing_args = TestingArgs()

        # Call the method under test and expect it to raise an exception
        with self.assertRaises(Exception) as context:
            testing_parser.validate_common_arguments(testing_args, None)

        # Check the exception message
        self.assertEqual(str(context.exception), "parser is None")

    #### Tests for validate_arguments ####

    @patch("run_all_tests.NonRemotePythonTest.validate_arguments")
    @patch("run_all_tests.RemotePythonTest.validate_arguments")
    @patch("run_all_tests.NonRemoteTest.validate_arguments")
    @patch("run_all_tests.Neo4jTest.validate_arguments")
    @patch("run_all_tests.RemoteTest.validate_arguments")
    @patch.object(TestingParser, "validate_common_arguments")
    def test_validate_arguments(
        self,
        mock_validate_common,
        mock_remote_test,
        mock_neo4j_test,
        mock_non_remote_test,
        mock_remote_python_test,
        mock_non_remote_python_test,
    ):
        testing_parser = TestingParser()
        testing_args = TestingArgs()
        parser = MagicMock(spec=argparse.ArgumentParser)

        # Set the type_of_test attribute to trigger different validation paths
        test_types = {
            "pt": mock_non_remote_python_test,
            "rp": mock_remote_python_test,
            "ut": mock_non_remote_test,
            "neo": mock_neo4j_test,
            "ru": mock_remote_test,
        }

        for test_type, mock_test in test_types.items():
            with self.subTest(test_type=test_type):
                testing_args.type_of_test = test_type

                # Call the method under test
                testing_parser.validate_arguments(testing_args, parser)

                # Check that the common validation method was called
                mock_validate_common.assert_called_once_with(testing_args, parser)

                # Check that the specific test type's validation method was called
                mock_test.assert_called_once_with(testing_args, parser)

                # Reset mocks for the next iteration
                mock_validate_common.reset_mock()
                mock_test.reset_mock()

    def test_validate_arguments_exception_handling(self):
        testing_parser = TestingParser()
        testing_args = TestingArgs()
        parser = MagicMock(spec=argparse.ArgumentParser)

        # Simulate an exception being raised during common argument validation
        with patch.object(
            testing_parser,
            "validate_common_arguments",
            side_effect=Exception("Common validation failed"),
        ):
            with self.assertRaises(Exception) as context:
                testing_parser.validate_arguments(testing_args, parser)
            self.assertIn(
                "validate_arguments() error: Common validation failed",
                str(context.exception),
            )

    #### Tests for fill_default_stop_testing_value ####
    @patch("builtins.print")
    def test_fill_default_stop_testing_value_for_googletest(self, mock_print):
        testing_parser = TestingParser()
        testing_args = TestingArgs()

        # Set attributes for a GoogleTest type
        testing_args.type_of_test = "ut"

        # Call the method under test
        result = testing_parser.fill_default_stop_testing_value(testing_args)

        # Assertions to check if the default value is set correctly
        self.assertFalse(result.stop_tests_on_failure)
        mock_print.assert_not_called()

        # Set stop_tests_on_failure to True and check print
        testing_args.stop_tests_on_failure = True
        result = testing_parser.fill_default_stop_testing_value(testing_args)
        mock_print.assert_called_once_with("Using", STOP_ON_FAILURE_FLAG)

    def test_fill_default_stop_testing_value_for_non_googletest(self):
        testing_parser = TestingParser()
        testing_args = TestingArgs()

        # Set attributes for a non-GoogleTest type
        testing_args.type_of_test = "non_gt_type"

        # Call the method under test
        result = testing_parser.fill_default_stop_testing_value(testing_args)

        # Assertions to check if the stop_tests_on_failure is set to None
        self.assertIsNone(result.stop_tests_on_failure)

    def test_fill_default_stop_testing_value_already_set(self):
        testing_parser = TestingParser()
        testing_args = TestingArgs()

        # Set attributes for a GoogleTest type with stop_tests_on_failure already set
        testing_args.type_of_test = "ut"
        testing_args.stop_tests_on_failure = True

        # Call the method under test
        result = testing_parser.fill_default_stop_testing_value(testing_args)

        # Assertions to check if the stop_tests_on_failure remains True
        self.assertTrue(result.stop_tests_on_failure)

    #### Tests for  _set_default_if_unset ####
    def test_set_default_if_unset_already_set(self):
        # Test that the method does not change an attribute that is already set
        parser = TestingParser()
        testing_args = TestingArgs()

        setattr(testing_args, "attribute_already_set", "existing_value")
        parser._set_default_if_unset(
            testing_args, "attribute_already_set", "default_value"
        )
        self.assertEqual(
            getattr(testing_args, "attribute_already_set"), "existing_value"
        )

    def test_set_default_if_unset_not_set(self):
        # Test that the method sets the default value if the attribute is not set
        parser = TestingParser()
        testing_args = TestingArgs()

        parser._set_default_if_unset(testing_args, "attribute_not_set", "default_value")
        self.assertEqual(getattr(testing_args, "attribute_not_set"), "default_value")

    def test_set_default_if_unset_none(self):
        # Test that the method sets the default value if the attribute is None
        parser = TestingParser()
        testing_args = TestingArgs()

        setattr(testing_args, "attribute_none", None)
        parser._set_default_if_unset(testing_args, "attribute_none", "default_value")
        self.assertEqual(getattr(testing_args, "attribute_none"), "default_value")

    def test_set_default_if_unset_empty_string(self):
        # Test that the method sets the default value if the attribute is an empty string
        parser = TestingParser()
        testing_args = TestingArgs()

        setattr(testing_args, "attribute_empty_string", "")
        parser._set_default_if_unset(
            testing_args, "attribute_empty_string", "default_value"
        )
        self.assertEqual(
            getattr(testing_args, "attribute_empty_string"), "default_value"
        )

    #### Tests for fill_default_tmp_dir_value ####
    def test_fill_default_tmp_dir_value_already_set(self):
        # Test that the method does not change the tmp_tests_dir if it is already set
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.tmp_tests_dir = "existing_dir"
        parser.fill_default_tmp_dir_value(testing_args)
        self.assertEqual(testing_args.tmp_tests_dir, "existing_dir")

    def test_fill_default_tmp_dir_value_not_set(self):
        # Test that the method sets the default tmp_tests_dir if it is not set
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.tmp_tests_dir = None
        parser.fill_default_tmp_dir_value(testing_args)
        self.assertEqual(testing_args.tmp_tests_dir, DEFAULT_TMP_DIR)

    def test_fill_default_tmp_dir_value_empty_string(self):
        # Test that the method sets the default tmp_tests_dir if it is an empty string
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.tmp_tests_dir = ""
        parser.fill_default_tmp_dir_value(testing_args)
        self.assertEqual(testing_args.tmp_tests_dir, DEFAULT_TMP_DIR)

    #### Tests for fill_default_stderr_filename() ####
    def test_fill_default_stderr_filename_already_set(self):
        # Test that the method does not change stderr_filename if it is already set
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.stderr_filename = "existing_stderr.log"
        parser.fill_default_stderr_filename(testing_args)
        self.assertEqual(testing_args.stderr_filename, "existing_stderr.log")

    def test_fill_default_stderr_filename_not_set(self):
        # Test that the method sets the default stderr_filename if it is not set
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.stderr_filename = None
        parser.fill_default_stderr_filename(testing_args)
        self.assertEqual(testing_args.stderr_filename, DEFAULT_TESTS_STDERR_FILENAME)

    def test_fill_default_stderr_filename_empty_string(self):
        # Test that the method sets the default stderr_filename if it is an empty string
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.stderr_filename = ""
        parser.fill_default_stderr_filename(testing_args)
        self.assertEqual(testing_args.stderr_filename, DEFAULT_TESTS_STDERR_FILENAME)

    #### Tests for fill_default_stdout_filename() ####
    def test_fill_default_stdout_filename_already_set(self):
        # Test that the method does not change stdout_filename if it is already set
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.stdout_filename = "existing_stdout.log"
        parser.fill_default_stdout_filename(testing_args)
        self.assertEqual(testing_args.stdout_filename, "existing_stdout.log")

    def test_fill_default_stdout_filename_not_set(self):
        # Test that the method sets the default stdout_filename if it is not set
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.stdout_filename = None
        parser.fill_default_stdout_filename(testing_args)
        self.assertEqual(testing_args.stdout_filename, DEFAULT_TESTS_STDOUT_FILENAME)

    def test_fill_default_stdout_filename_empty_string(self):
        # Test that the method sets the default stdout_filename if it is an empty string
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.stdout_filename = ""
        parser.fill_default_stdout_filename(testing_args)
        self.assertEqual(testing_args.stdout_filename, DEFAULT_TESTS_STDOUT_FILENAME)

    #### Tests for fill_default_udf_local_stderr_filename() ####
    def test_fill_default_udf_local_stderr_filename_already_set(self):
        # Test that the method does not change udf_local_stderr_filename if it is already set
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.udf_local_stderr_filename = "existing_stderr.log"
        parser.fill_default_udf_local_stderr_filename(testing_args)
        self.assertEqual(testing_args.udf_local_stderr_filename, "existing_stderr.log")

    def test_fill_default_udf_local_stderr_filename_not_set(self):
        # Test that the method sets the default udf_local_stderr_filename if it is not set
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.udf_local_stderr_filename = None
        parser.fill_default_udf_local_stderr_filename(testing_args)
        self.assertEqual(
            testing_args.udf_local_stderr_filename,
            run_all_tests.DEFAULT_UDF_LOCAL_STDERR_FILENAME,
        )

    def test_fill_default_udf_local_stderr_filename_empty_string(self):
        # Test that the method sets the default stdout_filename if it is an empty string
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.udf_local_stderr_filename = ""
        parser.fill_default_udf_local_stderr_filename(testing_args)
        self.assertEqual(
            testing_args.udf_local_stderr_filename,
            run_all_tests.DEFAULT_UDF_LOCAL_STDERR_FILENAME,
        )

    #### Tests for fill_default_udf_local_stdout_filename() ####
    def test_fill_default_udf_local_stdout_filename_already_set(self):
        # Test that the method does not change udf_local_stdout_filename if it is already set
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.udf_local_stdout_filename = "existing_stdout.log"
        parser.fill_default_udf_local_stdout_filename(testing_args)
        self.assertEqual(testing_args.udf_local_stdout_filename, "existing_stdout.log")

    def test_fill_default_udf_local_stdout_filename_not_set(self):
        # Test that the method sets the default udf_local_stdout_filename if it is not set
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.udf_local_stdout_filename = None
        parser.fill_default_udf_local_stdout_filename(testing_args)
        self.assertEqual(
            testing_args.udf_local_stdout_filename,
            run_all_tests.DEFAULT_UDF_LOCAL_STDOUT_FILENAME,
        )

    def test_fill_default_udf_local_stdout_filename_empty_string(self):
        # Test that the method sets the default udf_local_stdout_filename if it is an empty string
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.udf_local_stdout_filename = ""
        parser.fill_default_udf_local_stdout_filename(testing_args)
        self.assertEqual(
            testing_args.udf_local_stdout_filename,
            run_all_tests.DEFAULT_UDF_LOCAL_STDOUT_FILENAME,
        )

    #### Tests for fill_default_udf_server_stderr_filename() ####
    def test_fill_default_udf_server_stderr_filename_already_set(self):
        # Test that the method does not change udf_server_stderr_filename if it is already set
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.udf_server_stderr_filename = "existing_stderr.log"
        parser.fill_default_udf_server_stderr_filename(testing_args)
        self.assertEqual(testing_args.udf_server_stderr_filename, "existing_stderr.log")

    def test_fill_default_udf_server_stderr_filename_not_set(self):
        # Test that the method sets the default udf_server_stderr_filename if it is not set
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.udf_server_stderr_filename = None
        parser.fill_default_udf_server_stderr_filename(testing_args)
        self.assertEqual(
            testing_args.udf_server_stderr_filename,
            run_all_tests.DEFAULT_UDF_SERVER_STDERR_FILENAME,
        )

    def test_fill_default_udf_server_stderr_filename_empty_string(self):
        # Test that the method sets the default udf_server_stderr_filename if it is an empty string
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.udf_server_stderr_filename = ""
        parser.fill_default_udf_server_stderr_filename(testing_args)
        self.assertEqual(
            testing_args.udf_server_stderr_filename,
            run_all_tests.DEFAULT_UDF_SERVER_STDERR_FILENAME,
        )

    #### Tests for fill_default_udf_server_stdout_filename() ####
    def test_fill_default_udf_server_stdout_filename_already_set(self):
        # Test that the method does not change udf_server_stdout_filename if it is already set
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.udf_server_stdout_filename = "existing_stdout.log"
        parser.fill_default_udf_server_stdout_filename(testing_args)
        self.assertEqual(testing_args.udf_server_stdout_filename, "existing_stdout.log")

    def test_fill_default_udf_server_stdout_filename_not_set(self):
        # Test that the method sets the default udf_server_stdout_filename if it is not set
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.udf_server_stdout_filename = None
        parser.fill_default_udf_server_stdout_filename(testing_args)
        self.assertEqual(
            testing_args.udf_server_stdout_filename,
            run_all_tests.DEFAULT_UDF_SERVER_STDOUT_FILENAME,
        )

    def test_fill_default_udf_server_stdout_filename_empty_string(self):
        # Test that the method sets the default udf_server_stdout_filename if it is an empty string
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.udf_server_stdout_filename = ""
        parser.fill_default_udf_server_stdout_filename(testing_args)
        self.assertEqual(
            testing_args.udf_server_stdout_filename,
            run_all_tests.DEFAULT_UDF_SERVER_STDOUT_FILENAME,
        )

    #### Tests for fill_default_tls_stdout_filename() ####
    def test_fill_default_tls_stdout_filename_already_set(self):
        # Test that the method does not change tls_stdout_filename if it is already set
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.tls_stdout_filename = "existing_stdout.log"
        parser.fill_default_tls_stdout_filename(testing_args)
        self.assertEqual(testing_args.tls_stdout_filename, "existing_stdout.log")

    def test_fill_default_tls_stdout_filename_not_set(self):
        # Test that the method sets the default tls_stdout_filename if it is not set
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.tls_stdout_filename = None
        parser.fill_default_tls_stdout_filename(testing_args)
        self.assertEqual(
            testing_args.tls_stdout_filename, run_all_tests.DEFAULT_TLS_STDOUT_FILENAME
        )

    def test_fill_default_tls_stdout_filename_empty_string(self):
        # Test that the method sets the default tls_stdout_filename if it is an empty string
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.tls_stdout_filename = ""
        parser.fill_default_tls_stdout_filename(testing_args)
        self.assertEqual(
            testing_args.tls_stdout_filename, run_all_tests.DEFAULT_TLS_STDOUT_FILENAME
        )

    #### Tests for fill_default_tls_stderr_filename() ####
    def test_fill_default_tls_stderr_filename_already_set(self):
        # Test that the method does not change tls_stderr_filename if it is already set
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.tls_stderr_filename = "existing_stderr.log"
        parser.fill_default_tls_stderr_filename(testing_args)
        self.assertEqual(testing_args.tls_stderr_filename, "existing_stderr.log")

    def test_fill_default_tls_stderr_filename_not_set(self):
        # Test that the method sets the default tls_stderr_filename if it is not set
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.tls_stderr_filename = None
        parser.fill_default_tls_stderr_filename(testing_args)
        self.assertEqual(
            testing_args.tls_stderr_filename, run_all_tests.DEFAULT_TLS_STDERR_FILENAME
        )

    def test_fill_default_tls_stderr_filename_empty_string(self):
        # Test that the method sets the default tls_stderr_filename if it is an empty string
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.tls_stderr_filename = ""
        parser.fill_default_tls_stderr_filename(testing_args)
        self.assertEqual(
            testing_args.tls_stderr_filename, run_all_tests.DEFAULT_TLS_STDERR_FILENAME
        )

    #### Tests for fill_default_minio_stdout_filename() ####
    def test_fill_default_minio_stdout_filename_already_set(self):
        # Test that the method does not change minio_stdout_filename if it is already set
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.minio_stdout_filename = "existing_stdout.log"
        parser.fill_default_minio_stdout_filename(testing_args)
        self.assertEqual(testing_args.minio_stdout_filename, "existing_stdout.log")

    def test_fill_default_minio_stdout_filename_not_set(self):
        # Test that the method sets the default minio_stdout_filename if it is not set
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.minio_stdout_filename = None
        parser.fill_default_minio_stdout_filename(testing_args)
        self.assertEqual(
            testing_args.minio_stdout_filename,
            run_all_tests.DEFAULT_MINIO_STDOUT_FILENAME,
        )

    def test_fill_default_minio_stdout_filename_empty_string(self):
        # Test that the method sets the default minio_stdout_filename if it is an empty string
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.minio_stdout_filename = ""
        parser.fill_default_minio_stdout_filename(testing_args)
        self.assertEqual(
            testing_args.minio_stdout_filename,
            run_all_tests.DEFAULT_MINIO_STDOUT_FILENAME,
        )

    #### Tests for fill_default_minio_stderr_filename() ####
    def test_fill_default_minio_stderr_filename_already_set(self):
        # Test that the method does not change minio_stderr_filename if it is already set
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.minio_stderr_filename = "existing_stderr.log"
        parser.fill_default_minio_stderr_filename(testing_args)
        self.assertEqual(testing_args.minio_stderr_filename, "existing_stderr.log")

    def test_fill_default_minio_stderr_filename_not_set(self):
        # Test that the method sets the default minio_stderr_filename if it is not set
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.minio_stderr_filename = None
        parser.fill_default_minio_stderr_filename(testing_args)
        self.assertEqual(
            testing_args.minio_stderr_filename,
            run_all_tests.DEFAULT_MINIO_STDERR_FILENAME,
        )

    def test_fill_default_minio_stderr_filename_empty_string(self):
        # Test that the method sets the default minio_stderr_filename if it is an empty string
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.minio_stderr_filename = ""
        parser.fill_default_minio_stderr_filename(testing_args)
        self.assertEqual(
            testing_args.minio_stderr_filename,
            run_all_tests.DEFAULT_MINIO_STDERR_FILENAME,
        )

    #### Tests for fill_default_vdms_stdout_filename() ####
    def test_fill_default_vdms_stdout_filename_already_set(self):
        # Test that the method does not change vdms_stdout_filename if it is already set
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.vdms_stdout_filename = "existing_stdout.log"
        parser.fill_default_vdms_stdout_filename(testing_args)
        self.assertEqual(testing_args.vdms_stdout_filename, "existing_stdout.log")

    def test_fill_default_vdms_stdout_filename_not_set(self):
        # Test that the method sets the default vdms_stdout_filename if it is not set
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.vdms_stdout_filename = None
        parser.fill_default_vdms_stdout_filename(testing_args)
        self.assertEqual(
            testing_args.vdms_stdout_filename,
            run_all_tests.DEFAULT_VDMS_STDOUT_FILENAME,
        )

    def test_fill_default_vdms_stdout_filename_empty_string(self):
        # Test that the method sets the default vdms_stdout_filename if it is an empty string
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.vdms_stdout_filename = ""
        parser.fill_default_vdms_stdout_filename(testing_args)
        self.assertEqual(
            testing_args.vdms_stdout_filename,
            run_all_tests.DEFAULT_VDMS_STDOUT_FILENAME,
        )

    #### Tests for fill_default_vdms_stderr_filename() ####
    def test_fill_default_vdms_stderr_filename_already_set(self):
        # Test that the method does not change vdms_stderr_filename if it is already set
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.vdms_stderr_filename = "existing_stderr.log"
        parser.fill_default_vdms_stderr_filename(testing_args)
        self.assertEqual(testing_args.vdms_stderr_filename, "existing_stderr.log")

    def test_fill_default_vdms_stderr_filename_not_set(self):
        # Test that the method sets the default vdms_stderr_filename if it is not set
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.vdms_stderr_filename = None
        parser.fill_default_vdms_stderr_filename(testing_args)
        self.assertEqual(
            testing_args.vdms_stderr_filename,
            run_all_tests.DEFAULT_VDMS_STDERR_FILENAME,
        )

    def test_fill_default_vdms_stderr_filename_empty_string(self):
        # Test that the method sets the default vdms_stderr_filename if it is an empty string
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.vdms_stderr_filename = ""
        parser.fill_default_vdms_stderr_filename(testing_args)
        self.assertEqual(
            testing_args.vdms_stderr_filename,
            run_all_tests.DEFAULT_VDMS_STDERR_FILENAME,
        )

    #### Tests for fill_default_keep_value ####
    def test_fill_default_keep_value_keep_tmp_tests_dir_already_set(self):
        # Test that the method does not change keep_tmp_tests_dir if it is already set
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.keep_tmp_tests_dir = True
        parser.fill_default_keep_value(testing_args)
        self.assertTrue(testing_args.keep_tmp_tests_dir)

    def test_fill_default_keep_value_keep_tmp_tests_dir_not_set(self):
        # Test that the method sets the default keep_tmp_tests_dir if it is not set
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.keep_tmp_tests_dir = None
        parser.fill_default_keep_value(testing_args)
        self.assertFalse(testing_args.keep_tmp_tests_dir)

    #### Tests for fill_default_run_value ####
    def test_fill_default_run_value_already_set(self):
        # Test that the method does not change run if it is already set
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.run = False
        parser.fill_default_run_value(testing_args)
        self.assertFalse(testing_args.run)

    def test_fill_default_run_value_not_set(self):
        # Test that the method sets the default run if it is not set
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.run = None
        parser.fill_default_run_value(testing_args)
        self.assertTrue(testing_args.run)

    #### Tests for fill_common_default_arguments() ####

    def test_fill_common_default_arguments_success(self):
        # Mock the methods called within fill_common_default_arguments to ensure they are called
        parser = TestingParser()
        testing_args = TestingArgs()

        parser.fill_default_stop_testing_value = MagicMock(return_value=testing_args)
        parser.fill_default_tmp_dir_value = MagicMock(return_value=testing_args)
        parser.fill_default_stderr_filename = MagicMock(return_value=testing_args)
        parser.fill_default_stdout_filename = MagicMock(return_value=testing_args)
        parser.fill_default_keep_value = MagicMock(return_value=testing_args)
        parser.fill_default_run_value = MagicMock(return_value=testing_args)

        # Call the method under test
        result = parser.fill_common_default_arguments(testing_args)

        # Assert that all the mocked methods were called once
        parser.fill_default_stop_testing_value.assert_called_once_with(testing_args)
        parser.fill_default_tmp_dir_value.assert_called_once_with(testing_args)
        parser.fill_default_stderr_filename.assert_called_once_with(testing_args)
        parser.fill_default_stdout_filename.assert_called_once_with(testing_args)
        parser.fill_default_keep_value.assert_called_once_with(testing_args)
        parser.fill_default_run_value.assert_called_once_with(testing_args)

        # Assert that the result is the modified TestingArgs object
        self.assertEqual(result, testing_args)

    def test_fill_common_default_arguments_exception(self):
        # Use patch.object to mock methods on the TestingParser instance
        parser = TestingParser()
        testing_args = TestingArgs()

        with patch.object(parser, "fill_default_tmp_dir_value") as mock_tmp_dir:
            # Mock fill_default_tmp_dir_value to raise an exception
            mock_tmp_dir.side_effect = Exception("Test exception")

            # Call the method under test and assert that an exception is raised
            with self.assertRaises(Exception) as context:
                parser.fill_common_default_arguments(testing_args)

            # Assert that the exception message is as expected
            self.assertEqual(
                str(context.exception),
                "fill_common_default_arguments() error: Test exception",
            )

    #### Tests for fill_default_arguments ####
    def test_fill_default_arguments_common(self):
        parser = TestingParser()
        testing_args = TestingArgs()

        # Mock fill_common_default_arguments to ensure it is called
        parser.fill_common_default_arguments = MagicMock(return_value=testing_args)

        # Call the method under test
        result = parser.fill_default_arguments(testing_args)

        # Assert that fill_common_default_arguments was called once
        parser.fill_common_default_arguments.assert_called_once_with(testing_args)

        # Assert that the result is the modified TestingArgs object
        self.assertEqual(result, testing_args)

    @patch.object(NonRemotePythonTest, "fill_default_arguments")
    def test_fill_default_arguments_non_remote_python_test(
        self, mock_fill_default_arguments
    ):
        parser = TestingParser()
        testing_args = TestingArgs()

        # Set the type of test
        testing_args.type_of_test = "pt"

        # Mock the fill_common_default_arguments method to return the testing_args
        parser.fill_common_default_arguments = MagicMock(return_value=testing_args)

        # Configure the mock for NonRemotePythonTest.fill_default_arguments to return the testing_args
        mock_fill_default_arguments.return_value = testing_args

        # Call the method under test
        result = parser.fill_default_arguments(testing_args)

        # Assert that the fill_default_arguments method was called once with the testing_args instance
        mock_fill_default_arguments.assert_called_once_with(testing_args)

        # Assert that the result is the modified TestingArgs object with 'pt' as type_of_test
        self.assertEqual(result.type_of_test, "pt")

    @patch.object(RemotePythonTest, "fill_default_arguments")
    def test_fill_default_arguments_remote_python_test(
        self, mock_fill_default_arguments
    ):
        parser = TestingParser()
        testing_args = TestingArgs()

        # Set the type of test
        testing_args.type_of_test = "rp"

        # Mock the fill_common_default_arguments method to return the testing_args
        parser.fill_common_default_arguments = MagicMock(return_value=testing_args)

        # Configure the mock for RemotePythonTest.fill_default_arguments to return the testing_args
        mock_fill_default_arguments.return_value = testing_args

        # Call the method under test
        result = parser.fill_default_arguments(testing_args)

        # Assert that the fill_default_arguments method was called once with the testing_args instance
        mock_fill_default_arguments.assert_called_once_with(testing_args)

        # Assert that the result is the modified TestingArgs object with 'rp' as type_of_test
        self.assertEqual(result.type_of_test, "rp")

    @patch.object(NonRemoteTest, "fill_default_arguments")
    def test_fill_default_arguments_non_remote_test(self, mock_fill_default_arguments):
        parser = TestingParser()
        testing_args = TestingArgs()

        # Set the type of test
        testing_args.type_of_test = "ut"

        # Mock the fill_common_default_arguments method to return the testing_args
        parser.fill_common_default_arguments = MagicMock(return_value=testing_args)

        # Configure the mock for NonRemoteTest.fill_default_arguments to return the testing_args
        mock_fill_default_arguments.return_value = testing_args

        # Call the method under test
        result = parser.fill_default_arguments(testing_args)

        # Assert that the fill_default_arguments method was called once with the testing_args instance
        mock_fill_default_arguments.assert_called_once_with(testing_args)

        # Assert that the result is the modified TestingArgs object with 'ut' as type_of_test
        self.assertEqual(result.type_of_test, "ut")

    @patch.object(Neo4jTest, "fill_default_arguments")
    def test_fill_default_arguments_neo4j_test(self, mock_fill_default_arguments):
        parser = TestingParser()
        testing_args = TestingArgs()

        # Set the type of test
        testing_args.type_of_test = "neo"

        # Mock the fill_common_default_arguments method to return the testing_args
        parser.fill_common_default_arguments = MagicMock(return_value=testing_args)

        # Configure the mock for Neo4jTest.fill_default_arguments to return the testing_args
        mock_fill_default_arguments.return_value = testing_args

        # Call the method under test
        result = parser.fill_default_arguments(testing_args)

        # Assert that the fill_default_arguments method was called once with the testing_args instance
        mock_fill_default_arguments.assert_called_once_with(testing_args)

        # Assert that the result is the modified TestingArgs object with 'neo' as type_of_test
        self.assertEqual(result.type_of_test, "neo")

    @patch.object(RemoteTest, "fill_default_arguments")
    def test_fill_default_arguments_remote_test(self, mock_fill_default_arguments):
        parser = TestingParser()
        testing_args = TestingArgs()

        # Set the type of test
        testing_args.type_of_test = "ru"

        # Mock the fill_common_default_arguments method to return the testing_args
        parser.fill_common_default_arguments = MagicMock(return_value=testing_args)

        # Configure the mock for RemoteTest.fill_default_arguments to return the testing_args
        mock_fill_default_arguments.return_value = testing_args

        # Call the method under test
        result = parser.fill_default_arguments(testing_args)

        # Assert that the fill_default_arguments method was called once with the testing_args instance
        mock_fill_default_arguments.assert_called_once_with(testing_args)

        # Assert that the result is the modified TestingArgs object with 'ru' as type_of_test
        self.assertEqual(result.type_of_test, "ru")

    def test_fill_default_arguments_exception(self):
        parser = TestingParser()
        testing_args = TestingArgs()

        # Mock fill_common_default_arguments to raise an exception
        parser.fill_common_default_arguments = MagicMock(
            side_effect=Exception("Test exception")
        )

        # Call the method under test and assert that an exception is raised
        with self.assertRaises(Exception) as context:
            parser.fill_default_arguments(testing_args)

        # Assert that the exception message is as expected
        self.assertEqual(
            str(context.exception), "fill_default_arguments() error: Test exception"
        )

    #### Tests for translate_db_root_path_to_tmp_dir ####

    @patch("run_all_tests.os.path.exists", return_value=True)
    @patch(
        "builtins.open",
        new_callable=mock_open,
        read_data='{"db_root_path": "/var/lib/db"}',
    )
    def test_translate_db_root_path_to_tmp_dir_success(self, mock_file, mock_exists):
        # Test translate_db_root_path_to_tmp_dir when the config file exists and contains valid JSON

        testingArgs = TestingArgs()
        testingArgs.tmp_tests_dir = "/tmp/tests"
        testingArgs.tmp_config_files_for_vdms = [
            "/tmp/tests/config1",
            "/tmp/tests/config2",
        ]

        instance = TestingParser()

        instance.translate_db_root_path_to_tmp_dir(testingArgs)
        # Assert that the file was read and written to
        mock_file.assert_called()
        # Assert that the db_root_path was updated in the JSON data
        handle = mock_file()
        written_data = handle.write.call_args[0][0]
        updated_json_data = json.loads(written_data)
        self.assertEqual(updated_json_data["db_root_path"], "/tmp/tests/db")

    @patch("run_all_tests.os.path.exists", return_value=False)
    def test_translate_db_root_path_to_tmp_dir_file_not_exist(self, mock_exists):
        # Test translate_db_root_path_to_tmp_dir when a config file does not exist
        testingArgs = TestingArgs()
        testingArgs.tmp_tests_dir = "/tmp/tests"
        testingArgs.tmp_config_files_for_vdms = [
            "/tmp/tests/config1",
            "/tmp/tests/config2",
        ]

        instance = TestingParser()

        with self.assertRaises(Exception) as context:
            instance.translate_db_root_path_to_tmp_dir(testingArgs)
        # Assert the exception message is as expected
        self.assertIn(
            "does not exist or there is not access to it", str(context.exception)
        )

    @patch("run_all_tests.os.path.exists", return_value=True)
    @patch("builtins.open", new_callable=mock_open, read_data="invalid json")
    def test_translate_db_root_path_to_tmp_dir_invalid_json(
        self, mock_file, mock_exists
    ):
        # Test translate_db_root_path_to_tmp_dir when the config file contains invalid JSON
        testingArgs = TestingArgs()
        testingArgs.tmp_tests_dir = "/tmp/tests"
        testingArgs.tmp_config_files_for_vdms = [
            "/tmp/tests/config1",
            "/tmp/tests/config2",
        ]

        instance = TestingParser()

        with self.assertRaises(Exception) as context:
            instance.translate_db_root_path_to_tmp_dir(testingArgs)
        # Assert the exception message indicates a JSON parsing error
        self.assertIn(
            "translate_db_root_path_to_tmp_dir() Error:", str(context.exception)
        )
        self.assertIn(
            "Expecting value", str(context.exception)
        )  # Part of the JSON decode error message

    @patch("run_all_tests.os.path.exists", return_value=True)
    @patch("builtins.open", side_effect=Exception("File operation failed"))
    def test_translate_db_root_path_to_tmp_dir_file_exception(
        self, mock_file, mock_exists
    ):
        # Test translate_db_root_path_to_tmp_dir when an exception occurs during file operations
        testingArgs = TestingArgs()
        testingArgs.tmp_tests_dir = "/tmp/tests"
        testingArgs.tmp_config_files_for_vdms = [
            "/tmp/tests/config1",
            "/tmp/tests/config2",
        ]

        instance = TestingParser()

        with self.assertRaises(Exception) as context:
            instance.translate_db_root_path_to_tmp_dir(testingArgs)
        # Assert the exception message is as expected
        self.assertIn(
            "translate_db_root_path_to_tmp_dir() Error: File operation failed",
            str(context.exception),
        )

    #### Tests for create_tmp_config_files ####

    @patch("run_all_tests.shutil.copy2")
    @patch("run_all_tests.os.path.exists", return_value=True)
    def test_create_tmp_config_files_success(self, mock_exists, mock_copy2):
        # Test create_tmp_config_files when config files exist and are successfully copied
        testingArgs = TestingArgs()
        instance = TestingParser()
        testingArgs.config_files_for_vdms = ["/path/to/config1", "/path/to/config2"]
        testingArgs.tmp_tests_dir = "/tmp/tests"

        # Use the actual os.path.join function within the side_effect
        with patch("run_all_tests.os.path.join", side_effect=os.path.join):
            updated_testingArgs = instance.create_tmp_config_files(testingArgs)

        # Assert tmp_config_files_for_vdms is updated with temp config file paths
        self.assertEqual(len(updated_testingArgs.tmp_config_files_for_vdms), 2)
        # Assert that the correct calls to copy2 were made
        expected_calls = [
            unittest.mock.call("/path/to/config1", "/tmp/tests/config1"),
            unittest.mock.call("/path/to/config2", "/tmp/tests/config2"),
        ]
        mock_copy2.assert_has_calls(expected_calls, any_order=True)

    @patch("run_all_tests.shutil.copy2")
    @patch("run_all_tests.os.path.exists")
    def test_create_tmp_config_files_config_not_exist(self, mock_exists, mock_copy2):
        # Test create_tmp_config_files when a config file does not exist
        mock_exists.side_effect = lambda path: path == "/tmp/tests"
        testingArgs = TestingArgs()
        instance = TestingParser()
        testingArgs.config_files_for_vdms = ["/path/to/config1", "/path/to/config2"]
        testingArgs.tmp_tests_dir = "/tmp/tests"

        with self.assertRaises(Exception) as context:
            instance.create_tmp_config_files(testingArgs)
        # Assert the exception message is as expected
        self.assertIn("is not a valid path", str(context.exception))
        # Assert that no files were copied
        mock_copy2.assert_not_called()

    @patch("run_all_tests.shutil.copy2")
    @patch("run_all_tests.os.path.exists")
    def test_create_tmp_config_files_tmp_dir_not_exist(self, mock_exists, mock_copy2):
        # Test create_tmp_config_files when the temporary test directory does not exist
        # Mock os.path.exists to return True for config files and False for the tmp_tests_dir

        def side_effect(path):
            if path in ["/path/to/config1", "/path/to/config2"]:
                return True
            elif path == "/tmp/tests":
                return False
            return True  # Default case for other paths that might be checked

        mock_exists.side_effect = side_effect
        testingArgs = TestingArgs()
        testingArgs.config_files_for_vdms = ["/path/to/config1", "/path/to/config2"]
        testingArgs.tmp_tests_dir = "/tmp/tests"

        instance = TestingParser()

        with self.assertRaises(Exception) as context:
            instance.create_tmp_config_files(testingArgs)
        # Assert the exception message is as expected
        expected_message = (
            f"tmp_tests_dir: {testingArgs.tmp_tests_dir} is not a valid dir"
        )
        self.assertIn(expected_message, str(context.exception))
        # Assert that no files were copied
        mock_copy2.assert_not_called()

    @patch("run_all_tests.os.path.exists", return_value=True)
    def test_create_tmp_config_files_no_config_files_for_vdms(self, mock_copy2):
        # Test create_tmp_config_files when an exception occurs during the shutil.copy2 operation
        testingArgs = TestingArgs()
        testingArgs.config_files_for_vdms = None
        testingArgs.tmp_tests_dir = "/tmp/tests"
        instance = TestingParser()

        with self.assertRaises(Exception) as context:
            instance.create_tmp_config_files(testingArgs)
        # Assert the exception message is as expected
        self.assertIn(
            "create_tmp_config_files() Error: config_files_for_vdms value is invalid",
            str(context.exception),
        )
        # Assert that the exception was raised during the copy operation
        mock_copy2.assert_called_once()

    #### Tests for create_dirs ####
    @patch("run_all_tests.os.makedirs")
    @patch("run_all_tests.os.path.exists", return_value=False)
    def test_create_dirs_not_exist(self, mock_exists, mock_makedirs):
        # Test create_dirs when directories do not exist
        dirs = ["/path/to/dir1", "/path/to/dir2"]
        instance = TestingParser()
        instance.create_dirs(dirs)
        # Assert os.makedirs was called for each directory
        mock_makedirs.assert_has_calls(
            [unittest.mock.call(dir) for dir in dirs], any_order=True
        )

    @patch("run_all_tests.os.makedirs")
    @patch("run_all_tests.shutil.rmtree")
    @patch("run_all_tests.os.path.exists", return_value=True)
    def test_create_dirs_already_exist(self, mock_exists, mock_rmtree, mock_makedirs):
        # Test create_dirs when directories already exist
        dirs = ["/path/to/existing/dir1", "/path/to/existing/dir2"]
        instance = TestingParser()
        instance.create_dirs(dirs)
        # Assert shutil.rmtree was called for each existing directory
        mock_rmtree.assert_has_calls(
            [unittest.mock.call(dir, ignore_errors=False) for dir in dirs],
            any_order=True,
        )
        # Assert os.makedirs was called for each directory
        mock_makedirs.assert_has_calls(
            [unittest.mock.call(dir) for dir in dirs], any_order=True
        )

    @patch("run_all_tests.os.makedirs")
    @patch(
        "run_all_tests.shutil.rmtree", side_effect=Exception("Mocked rmtree exception")
    )
    @patch("run_all_tests.os.path.exists", return_value=True)
    def test_create_dirs_rmtree_exception(
        self, mock_exists, mock_rmtree, mock_makedirs
    ):
        # Test create_dirs when an exception occurs during shutil.rmtree
        dirs = ["/path/to/existing/dir1"]
        instance = TestingParser()
        with self.assertRaises(Exception) as context:
            instance.create_dirs(dirs)
        # Assert the exception message is as expected
        self.assertIn(
            "create_dirs() Error: Mocked rmtree exception", str(context.exception)
        )

    @patch(
        "run_all_tests.os.makedirs", side_effect=Exception("Mocked makedirs exception")
    )
    @patch("run_all_tests.os.path.exists", return_value=False)
    def test_create_dirs_makedirs_exception(self, mock_exists, mock_makedirs):
        # Test create_dirs when an exception occurs during os.makedirs
        dirs = ["/path/to/new/dir1"]
        instance = TestingParser()
        with self.assertRaises(Exception) as context:
            instance.create_dirs(dirs)
        # Assert the exception message is as expected
        self.assertIn(
            "create_dirs() Error: Mocked makedirs exception", str(context.exception)
        )

    #### Tests for setup ####
    @patch("run_all_tests.TestingParser.create_dirs")
    @patch("run_all_tests.TestingParser.create_tmp_config_files")
    @patch("run_all_tests.TestingParser.translate_db_root_path_to_tmp_dir")
    def test_setup(self, mock_translate, mock_create_tmp, mock_create_dirs):
        parser = TestingParser()
        testing_args = TestingArgs()
        testing_args.tmp_tests_dir = "/path/to/tmp"
        testing_args.type_of_test = "ut"  # Example of valid test type
        testing_args.config_files_for_vdms = ["/path/to/config_file.json"]

        # Set up mocks for the methods called within setup
        mock_create_dirs.return_value = None
        # Use a lambda expression as side_effect to update tmp_config_files_for_vdms
        mock_create_tmp.side_effect = (
            lambda testing_args: setattr(
                testing_args,
                "tmp_config_files_for_vdms",
                ["/path/to/tmp/tmp_config_file.json"],
            )
            or testing_args
        )
        mock_translate.return_value = None

        # Call the method under test
        parser.setup(testing_args)

        # Assert that create_dirs was called with the tmp_tests_dir
        mock_create_dirs.assert_called_once_with([testing_args.tmp_tests_dir])

        # Assert that create_tmp_config_files was called with testing_args
        mock_create_tmp.assert_called_once_with(testing_args)

        # Assert that translate_db_root_path_to_tmp_dir was called with testing_args
        mock_translate.assert_called_once_with(testing_args)

        # Assert that the tmp_config_files_for_vdms attribute was updated
        self.assertEqual(
            testing_args.tmp_config_files_for_vdms,
            ["/path/to/tmp/tmp_config_file.json"],
        )

    #### Tests for execute_test ####
    @patch("run_all_tests.TestingParser.setup")
    @patch("run_all_tests.NonRemotePythonTest.run")
    def test_execute_test_non_remote_python(self, mock_run, mock_setup):
        parser = TestingParser()
        testing_args = TestingArgs()

        # Set the type of test
        testing_args.type_of_test = "pt"
        # Configure the mock for RemoteTest.fill_default_arguments to return the testing_args
        mock_setup.return_value = testing_args

        # Call the method under test
        parser.execute_test(testing_args)

        # Assert that setup and cleanup were called
        mock_setup.assert_called_once_with(testing_args)

        # Assert that NonRemotePythonTest.run was called
        mock_run.assert_called_once_with(testing_args)

    @patch("run_all_tests.TestingParser.setup")
    @patch("run_all_tests.RemotePythonTest.run")
    def test_execute_test_remote_python(self, mock_run, mock_setup):
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.type_of_test = "rp"
        mock_setup.return_value = testing_args
        parser.execute_test(testing_args)
        mock_setup.assert_called_once_with(testing_args)

    @patch("run_all_tests.TestingParser.setup")
    @patch("run_all_tests.NonRemoteTest.run")
    def test_execute_test_non_remote(self, mock_run, mock_setup):
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.type_of_test = "ut"
        mock_setup.return_value = testing_args

        parser.execute_test(testing_args)
        mock_setup.assert_called_once_with(testing_args)

    @patch("run_all_tests.TestingParser.setup")
    @patch("run_all_tests.Neo4jTest.run")
    def test_execute_test_neo4j(self, mock_run, mock_setup):
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.type_of_test = "neo"
        mock_setup.return_value = testing_args

        parser.execute_test(testing_args)
        mock_setup.assert_called_once_with(testing_args)

    @patch("run_all_tests.TestingParser.setup")
    @patch("run_all_tests.RemoteTest.run")
    def test_execute_test_remote(self, mock_run, mock_setup):
        parser = TestingParser()
        testing_args = TestingArgs()

        testing_args.type_of_test = "ru"
        mock_setup.return_value = testing_args

        parser.execute_test(testing_args)
        mock_setup.assert_called_once_with(testing_args)

    @patch("run_all_tests.TestingParser.setup")
    def test_execute_test_exception(self, mock_setup):
        parser = TestingParser()
        testing_args = TestingArgs()

        parser = TestingParser()
        testing_args = TestingArgs()

        # Simulate an exception during setup
        mock_setup.side_effect = Exception("Setup failed")

        # Call the method under test and assert that an exception is raised
        with self.assertRaises(Exception) as context:
            parser.execute_test(testing_args)

        # Assert that the exception message is as expected
        self.assertEqual(str(context.exception), "execute_test() Error: Setup failed")

    #### Tests for merge_testing_args ####
    def test_merge_testing_args_priority_to_cmd_line(self):
        parser = TestingParser()

        # Create two TestingArgs objects with different values
        args_from_json = TestingArgs()
        args_from_json.test_name = "Test from JSON"
        args_from_json.minio_username = "json_user"

        args_from_cmd_line = TestingArgs()
        args_from_cmd_line.test_name = "Test from CMD"
        args_from_cmd_line.minio_port = 9000

        # Call the method under test
        merged_args = parser.merge_testing_args(args_from_json, args_from_cmd_line)

        # Assert that the command line arguments have priority
        self.assertEqual(merged_args.test_name, "Test from CMD")
        self.assertEqual(merged_args.minio_username, "json_user")
        self.assertEqual(merged_args.minio_port, 9000)

    def test_merge_testing_args_none_json_args(self):
        parser = TestingParser()

        # Create a TestingArgs object with values
        args_from_cmd_line = TestingArgs()
        args_from_cmd_line.test_name = "Test from CMD"
        args_from_cmd_line.minio_port = 9000

        # Call the method under test with None for JSON args
        merged_args = parser.merge_testing_args(None, args_from_cmd_line)

        # Assert that the merged args are the same as the command line args
        self.assertEqual(merged_args.test_name, "Test from CMD")
        self.assertEqual(merged_args.minio_port, 9000)

    def test_merge_testing_args_none_cmd_line_args(self):
        parser = TestingParser()

        # Create a TestingArgs object with values
        args_from_json = TestingArgs()
        args_from_json.test_name = "Test from JSON"
        args_from_json.minio_username = "json_user"

        # Call the method under test with None for command line args
        merged_args = parser.merge_testing_args(args_from_json, None)

        # Assert that the merged args are the same as the JSON args
        self.assertEqual(merged_args.test_name, "Test from JSON")
        self.assertEqual(merged_args.minio_username, "json_user")

    def test_merge_testing_args_both_none(self):
        parser = TestingParser()

        # Call the method under test with None for both JSON and command line args
        merged_args = parser.merge_testing_args(None, None)

        # Assert that the merged args are None
        self.assertIsNone(merged_args)

    def test_merge_testing_args_json_args_only(self):
        parser = TestingParser()

        # Create a TestingArgs object with values only in the JSON args
        args_from_json = TestingArgs()
        args_from_json.test_name = "Test from JSON"
        args_from_json.minio_username = "json_user"

        # Call the method under test with None for command line args
        merged_args = parser.merge_testing_args(args_from_json, TestingArgs())

        # Assert that the merged args contain the JSON args
        self.assertEqual(merged_args.test_name, "Test from JSON")
        self.assertEqual(merged_args.minio_username, "json_user")

    def test_merge_testing_args_cmd_line_args_only(self):
        parser = TestingParser()

        # Create a TestingArgs object with values only in the command line args
        args_from_cmd_line = TestingArgs()
        args_from_cmd_line.test_name = "Test from CMD"
        args_from_cmd_line.minio_port = 9000

        # Call the method under test with None for JSON args
        merged_args = parser.merge_testing_args(TestingArgs(), args_from_cmd_line)

        # Assert that the merged args contain the command line args
        self.assertEqual(merged_args.test_name, "Test from CMD")
        self.assertEqual(merged_args.minio_port, 9000)

    def test_merge_testing_args_mixed_attributes(self):
        parser = TestingParser()

        # Create TestingArgs objects with different attributes set
        args_from_json = TestingArgs()
        args_from_json.test_name = "Test from JSON"
        args_from_json.minio_username = "json_user"

        args_from_cmd_line = TestingArgs()
        args_from_cmd_line.minio_port = 9000
        args_from_cmd_line.minio_alias_name = "cmd_alias"

        # Call the method under test with mixed attributes
        merged_args = parser.merge_testing_args(args_from_json, args_from_cmd_line)

        # Assert that the merged args contain the correct attributes
        self.assertEqual(merged_args.test_name, "Test from JSON")  # From JSON args
        self.assertEqual(merged_args.minio_username, "json_user")  # From JSON args
        self.assertEqual(merged_args.minio_port, 9000)  # From command line args
        self.assertEqual(
            merged_args.minio_alias_name, "cmd_alias"
        )  # From command line args

    @patch("run_all_tests.copy.deepcopy")
    def test_mergeTestingArgs_deepcopy_exception(self, mock_deepcopy):
        parser = TestingParser()

        # Create a mock TestingArgs object with valid data
        testingArgsFromJSONFile = TestingArgs()
        testingArgsFromJSONFile.some_attribute = "value_from_json"

        # Create a mock TestingArgs object for command line arguments
        testingArgsFromCmdLine = TestingArgs()
        testingArgsFromCmdLine.some_attribute = "value_from_cmd"

        # Configure the mock to raise an exception when called
        mock_deepcopy.side_effect = Exception("Mocked deepcopy exception")

        # Test that an exception is raised when deepcopy is mocked to raise an exception
        with self.assertRaises(Exception) as context:
            parser.merge_testing_args(testingArgsFromJSONFile, testingArgsFromCmdLine)

        # Check that the exception message is as expected
        self.assertTrue(
            "merge_testing_args() Error: Mocked deepcopy exception"
            in str(context.exception)
        )

        # Assert that deepcopy was called
        mock_deepcopy.assert_called_once_with(testingArgsFromJSONFile)

    #### Tests for convert_to_absolute_paths ####
    @patch("run_all_tests.os.path.isabs", return_value=False)
    @patch("run_all_tests.os.path.abspath", side_effect=lambda x: f"/abs/{x}")
    def test_convert_to_absolute_paths(self, mock_abspath, mock_isabs):

        testing_parser = TestingParser()
        testing_args = TestingArgs()

        # Set relative paths for testingArgs attributes
        testing_args.tmp_tests_dir = "relative/tmp_tests_dir"
        testing_args.config_files_for_vdms = [
            "relative/config1.json",
            "relative/config2.json",
        ]
        testing_args.tmp_config_files_for_vdms = [
            "relative/tmp_config1.json",
            "relative/tmp_config2.json",
        ]
        testing_args.minio_app_path = "relative/minio_app_path"
        testing_args.vdms_app_path = "relative/vdms_app_path"
        testing_args.googletest_path = "relative/googletest_path"

        # Call the method
        converted_args = testing_parser.convert_to_absolute_paths(testing_args)

        # Check if the paths were converted to absolute paths
        self.assertEqual(converted_args.tmp_tests_dir, "/abs/relative/tmp_tests_dir")
        self.assertEqual(
            converted_args.config_files_for_vdms,
            ["/abs/relative/config1.json", "/abs/relative/config2.json"],
        )
        self.assertEqual(
            converted_args.tmp_config_files_for_vdms,
            ["/abs/relative/tmp_config1.json", "/abs/relative/tmp_config2.json"],
        )
        self.assertEqual(converted_args.minio_app_path, "/abs/relative/minio_app_path")
        self.assertEqual(converted_args.vdms_app_path, "/abs/relative/vdms_app_path")
        self.assertEqual(
            converted_args.googletest_path, "/abs/relative/googletest_path"
        )

        # Check if the os.path functions were called correctly
        mock_isabs.assert_called()
        mock_abspath.assert_called()

    #### Tests for print_help ####
    def test_print_help(self):
        parser = TestingParser()
        arg_parser = argparse.ArgumentParser(description="Test parser")

        # Mock the print_help method of the ArgumentParser
        arg_parser.print_help = MagicMock()

        # Call the method under test
        parser.print_help(arg_parser)

        # Assert that print_help was called
        arg_parser.print_help.assert_called_once()

    #### Tests for print_error ####
    def test_print_error(self):
        parser = TestingParser()
        arg_parser = argparse.ArgumentParser(description="Test parser")

        # Mock the error method of the ArgumentParser to raise SystemExit
        arg_parser.error = MagicMock(side_effect=SystemExit)

        # Call the method under test with a sample error message
        error_message = "An error occurred"
        with self.assertRaises(SystemExit):
            parser.print_error(error_message, arg_parser)

        # Assert that error was called with the correct message
        arg_parser.error.assert_called_once_with(error_message)

    #### Tests for main() ####

    @patch("run_all_tests.TestingParser")
    @patch("run_all_tests.kill_processes_by_object")
    @patch("run_all_tests.close_log_files")
    @patch("run_all_tests.cleanup")
    @patch("run_all_tests.exit")
    @patch("run_all_tests.print")
    def test_main_success(
        self,
        mock_print,
        mock_exit,
        mock_cleanup,
        mock_close_log_files,
        mock_kill_processes_by_object,
        mock_TestingParser,
    ):
        run_all_tests.DEBUG_MODE = True
        testing_args = TestingArgs()
        testing_args.run = True
        # Create a TestingParser instance with mocked methods
        mock_parser_instance = MagicMock(spec=TestingParser)
        mock_args = MagicMock(run=True, json=None)  # No JSON file provided
        mock_parser_instance.parse_arguments.return_value = mock_args
        mock_parser_instance.convert_namespace_to_testing_args.return_value = (
            testing_args
        )
        mock_parser_instance.merge_testing_args.return_value = testing_args
        mock_parser_instance.convert_to_absolute_paths.return_value = testing_args
        mock_parser_instance.validate_arguments.return_value = None
        mock_parser_instance.fill_default_arguments.return_value = testing_args
        mock_parser_instance.execute_test.return_value = None
        mock_parser_instance.get_parser.return_value = MagicMock()
        mock_TestingParser.return_value = mock_parser_instance

        # Call the main function
        main()

        # Check if the necessary methods were called
        mock_parser_instance.parse_arguments.assert_called_once()
        mock_parser_instance.convert_namespace_to_testing_args.assert_called_once()
        mock_parser_instance.merge_testing_args.assert_called_once()
        mock_parser_instance.convert_to_absolute_paths.assert_called_once()
        mock_parser_instance.validate_arguments.assert_called_once()
        mock_parser_instance.fill_default_arguments.assert_called_once()
        mock_parser_instance.execute_test.assert_called_once()

        # Check if the cleanup functions were called
        mock_kill_processes_by_object.assert_called_once()
        mock_close_log_files.assert_called_once()
        mock_cleanup.assert_called_once()

        # Check if the exit function was called with the correct status code
        mock_exit.assert_called_once_with(0)

    @patch("run_all_tests.TestingParser.execute_test")
    @patch("run_all_tests.TestingParser.fill_default_arguments")
    @patch("run_all_tests.TestingParser.validate_arguments")
    @patch("run_all_tests.TestingParser.convert_to_absolute_paths")
    @patch("run_all_tests.TestingParser.merge_testing_args")
    @patch("run_all_tests.TestingParser.convert_namespace_to_testing_args")
    @patch("run_all_tests.TestingParser.read_json_config_file")
    @patch("run_all_tests.TestingParser.parse_arguments")
    @patch("run_all_tests.TestingParser.get_parser")
    @patch("run_all_tests.kill_processes_by_object")
    @patch("run_all_tests.close_log_files")
    @patch("run_all_tests.cleanup")
    @patch("run_all_tests.exit")
    @patch("run_all_tests.print")
    def test_main_with_json_config(
        self,
        mock_print,
        mock_exit,
        mock_cleanup,
        mock_close_log_files,
        mock_kill_processes_by_object,
        mock_get_parser,
        mock_parse_arguments,
        mock_read_json_config_file,
        mock_convert_namespace_to_testing_args,
        mock_merge_testing_args,
        mock_convert_to_absolute_paths,
        mock_validate_arguments,
        mock_fill_default_arguments,
        mock_execute_test,
    ):
        # Set up the mock for parse_arguments to return an object with a json attribute
        mock_args = MagicMock()
        mock_args.json = "config.json"
        mock_parse_arguments.return_value = mock_args

        # Set up the mock for get_parser to return a MagicMock object
        mock_parser = MagicMock()
        mock_get_parser.return_value = mock_parser

        # Create instances of TestingArgs and set attributes manually
        testing_args_from_json = TestingArgs()
        testing_args_from_cmd_line = TestingArgs()
        merged_testing_args = TestingArgs()
        merged_testing_args.run = True

        # Set up the other mocks to return the TestingArgs instances
        mock_read_json_config_file.return_value = testing_args_from_json
        mock_convert_namespace_to_testing_args.return_value = testing_args_from_cmd_line
        mock_merge_testing_args.return_value = merged_testing_args
        mock_convert_to_absolute_paths.return_value = merged_testing_args
        mock_validate_arguments.return_value = None
        mock_fill_default_arguments.return_value = merged_testing_args
        mock_execute_test.return_value = None

        # Call the main function
        main()

        # Check if the read_json_config_file method was called with the provided JSON file
        mock_read_json_config_file.assert_called_once_with("config.json", mock_parser)

        # Check if the print statements for JSON file loading were made
        mock_print.assert_any_call(
            f"Note: -j/--json argument was provided: config.json"
        )
        mock_print.assert_any_call(
            "\tIf there are other arguments in the command line then they will have a higher priority than the ones found in the JSON file"
        )

        # Check if the necessary methods were called
        mock_parse_arguments.assert_called_once()
        mock_convert_namespace_to_testing_args.assert_called_once()
        mock_merge_testing_args.assert_called_once()
        mock_convert_to_absolute_paths.assert_called_once()
        mock_validate_arguments.assert_called_once()
        mock_fill_default_arguments.assert_called_once()
        mock_execute_test.assert_called_once()

        # Check if the cleanup functions were called
        mock_kill_processes_by_object.assert_called_once()
        mock_close_log_files.assert_called_once()
        mock_cleanup.assert_called_once()

    @patch("run_all_tests.TestingParser.execute_test")
    @patch("run_all_tests.TestingParser.fill_default_arguments")
    @patch("run_all_tests.TestingParser.validate_arguments")
    @patch("run_all_tests.TestingParser.convert_to_absolute_paths")
    @patch("run_all_tests.TestingParser.merge_testing_args")
    @patch("run_all_tests.TestingParser.convert_namespace_to_testing_args")
    @patch("run_all_tests.TestingParser.read_json_config_file")
    @patch("run_all_tests.TestingParser.parse_arguments")
    @patch("run_all_tests.TestingParser.get_parser")
    @patch("run_all_tests.kill_processes_by_object")
    @patch("run_all_tests.close_log_files")
    @patch("run_all_tests.cleanup")
    @patch("run_all_tests.exit")
    @patch("run_all_tests.print")
    def test_main_without_run(
        self,
        mock_print,
        mock_exit,
        mock_cleanup,
        mock_close_log_files,
        mock_kill_processes_by_object,
        mock_get_parser,
        mock_parse_arguments,
        mock_read_json_config_file,
        mock_convert_namespace_to_testing_args,
        mock_merge_testing_args,
        mock_convert_to_absolute_paths,
        mock_validate_arguments,
        mock_fill_default_arguments,
        mock_execute_test,
    ):
        # Set up the mock for parse_arguments to return an object with run set to False
        mock_args = MagicMock()
        mock_args.run = False
        mock_parse_arguments.return_value = mock_args

        # Set up the mock for get_parser to return a MagicMock object
        mock_parser = MagicMock()
        mock_get_parser.return_value = mock_parser

        # Create an instance of TestingArgs and set attributes manually
        testing_args = TestingArgs()
        testing_args.run = False

        # Set up the other mocks to return the TestingArgs instance
        mock_read_json_config_file.return_value = testing_args
        mock_convert_namespace_to_testing_args.return_value = testing_args
        mock_merge_testing_args.return_value = testing_args
        mock_convert_to_absolute_paths.return_value = testing_args
        mock_validate_arguments.return_value = None
        mock_fill_default_arguments.return_value = testing_args
        mock_execute_test.return_value = None

        # Call the main function
        main()

        # Check if the execute_test method was called
        mock_execute_test.assert_called()

        # Check if the cleanup functions were not called
        mock_kill_processes_by_object.assert_not_called()
        mock_close_log_files.assert_not_called()
        mock_cleanup.assert_not_called()

        # Check if the warning print statement was made
        mock_print.assert_any_call("Warning: --run flag is set to False")

        # Check if the exit function was called with the correct status code
        mock_exit.assert_called_once_with(0)

    @patch("run_all_tests.TestingParser")
    @patch("run_all_tests.kill_processes_by_object")
    @patch("run_all_tests.close_log_files")
    @patch("run_all_tests.cleanup")
    @patch("run_all_tests.exit")
    @patch("run_all_tests.print")
    def test_main_exception(
        self,
        mock_print,
        mock_exit,
        mock_cleanup,
        mock_close_log_files,
        mock_kill_processes_by_object,
        mock_TestingParser,
    ):
        # Create a MagicMock for the parser object
        mock_parser = MagicMock()

        # Set up the mock for TestingParser
        mock_parser_instance = MagicMock(spec=TestingParser)
        mock_parser_instance.parse_arguments.side_effect = Exception("Parsing failed")
        mock_parser_instance.get_parser.return_value = mock_parser
        mock_TestingParser.return_value = mock_parser_instance

        # Call the main function and catch the exception
        try:
            main()
        except Exception as e:
            # Check the exception message
            self.assertIn("Parsing failed", str(e))

        # Check if the cleanup functions were called
        mock_kill_processes_by_object.assert_called_once()
        mock_close_log_files.assert_called_once()
        mock_cleanup.assert_called_once()

        # Check if the print_error method was called with the correct message and parser object
        mock_parser_instance.print_error.assert_called_once_with(
            "main() Error: Parsing failed", mock_parser
        )

        # Check if the exit function was called with status code 1
        mock_exit.assert_called_once_with(1)


if __name__ == "__main__":
    unittest.main()
