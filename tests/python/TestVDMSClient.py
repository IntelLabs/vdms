#
# The MIT License
#
# @copyright Copyright (c) 2023 Intel Corporation
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

# The following tests test the vdms class found in the vdms.py file
import vdms
import unittest
from io import StringIO
from unittest.mock import patch


class TestVDMSClient(unittest.TestCase):
    port = 55565
    aws_port = 55564
    hostname = "localhost"
    assigned_port = port

    def setUp(self):
        # Get the port used to connect to the server
        db = self.create_db_connection()
        if db is not None:
            db.disconnect()

    def create_db_connection(self):
        db = vdms.vdms()
        connected = False
        try:
            connected = db.connect(self.hostname, self.port)
            self.assigned_port = self.port
        except Exception as e:
            if e.strerror == "Connection refused":
                try:
                    # Try to connect to the AWS/MinIO port used by the config files
                    if connected is False:
                        aws_connected = db.connect(self.hostname, self.aws_port)
                        if aws_connected is False:
                            print("create_db_connection() failed")
                            self.assigned_port = None
                            return None
                        else:
                            self.assigned_port = self.aws_port
                            connected = True
                except Exception as e:
                    print("create_db_connection() second attempt failed with exception")
            else:
                print("create_db_connection() first attempt failed with exception")

        return db

    def test_vdms_existing_connection(self):
        # Initialize
        # VDMS Server Info
        db = vdms.vdms()

        # Execute the test
        connected = db.connect(self.hostname, self.assigned_port)

        # Try to connect when it is already connected
        connected_again = db.connect(self.hostname, self.assigned_port)

        # Check results
        self.assertTrue(connected)
        self.assertFalse(connected_again)

        # Cleanup
        disconnected = db.disconnect()
        self.assertTrue(disconnected)

    def test_vdms_non_existing_connection(self):
        # Initialize
        db = vdms.vdms()

        # Execute the test
        disconnected = db.disconnect()

        # Check results
        self.assertFalse(disconnected)

    def test_vdms_non_json_query(self):
        # Initialize
        # VDMS Server Info
        db = vdms.vdms()
        query = "Non JSON value"
        expected_info = "Error parsing the query, ill formed JSON"
        expected_status = -1
        expected_command = "Transaction"

        # Execute the test
        connected = db.connect(self.hostname, self.assigned_port)
        result = db.query(query)

        # Check results
        self.assertEqual(expected_command, result[0][0]["FailedCommand"])
        self.assertEqual(expected_info, result[0][0]["info"])
        self.assertEqual(expected_status, result[0][0]["status"])
        self.assertTrue(connected)

        # Cleanup
        disconnected = db.disconnect()
        self.assertTrue(disconnected)

    def test_vdms_query_disconnected(self):
        # Initialize
        db = vdms.vdms()
        query = "{'test': 'test'}"
        expected_result = "NOT CONNECTED"

        # Execute the test
        result = db.query(query)
        self.assertEqual(result, expected_result)

    def test_vdms_get_last_response(self):
        # Initialize
        # VDMS Server Info
        db = vdms.vdms()
        query = "Non JSON value"
        expected_info = "Error parsing the query, ill formed JSON"
        expected_status = -1
        expected_command = "Transaction"

        # Execute the test
        connected = db.connect(self.hostname, self.assigned_port)
        result = db.query(query)
        last_response = db.get_last_response()

        # Check results
        self.assertEqual(expected_command, result[0][0]["FailedCommand"])
        self.assertEqual(expected_info, result[0][0]["info"])
        self.assertEqual(expected_status, result[0][0]["status"])
        self.assertEqual(expected_command, last_response[0]["FailedCommand"])
        self.assertEqual(expected_info, last_response[0]["info"])
        self.assertEqual(expected_status, last_response[0]["status"])
        self.assertTrue(connected)

        # Cleanup
        disconnected = db.disconnect()
        self.assertTrue(disconnected)

    def test_vdms_get_last_response_str(self):
        # Initialize
        # VDMS Server Info
        db = vdms.vdms()
        query = "Non JSON value"
        expected_info = "Error parsing the query, ill formed JSON"
        expected_status = -1
        expected_command = "Transaction"
        expected_response = '[\n    {\n        "FailedCommand": "Transaction",\n        "info": "Error parsing the query, ill formed JSON",\n        "status": -1\n    }\n]'

        # Execute the test
        connected = db.connect(self.hostname, self.assigned_port)
        result = db.query(query)
        last_response_str = db.get_last_response_str()

        # Check results
        self.assertEqual(expected_command, result[0][0]["FailedCommand"])
        self.assertEqual(expected_info, result[0][0]["info"])
        self.assertEqual(expected_status, result[0][0]["status"])
        self.assertEqual(expected_response, last_response_str)
        self.assertTrue(connected)

        # Cleanup
        disconnected = db.disconnect()
        self.assertTrue(disconnected)

    def test_vdms_print_last_response(self):
        # Initialize
        # VDMS Server Info
        db = vdms.vdms()
        query = "Non JSON value"
        expected_info = "Error parsing the query, ill formed JSON"
        expected_status = -1
        expected_command = "Transaction"
        expected_output = '[\n    {\n        "FailedCommand": "Transaction",\n        "info": "Error parsing the query, ill formed JSON",\n        "status": -1\n    }\n]'

        # Execute the test
        connected = db.connect(self.hostname, self.assigned_port)
        result = db.query(query)
        with patch("sys.stdout", new=StringIO()) as fake_out:
            db.print_last_response()
            fake_output = fake_out.getvalue()

        # Check results
        self.assertEqual(fake_output.splitlines(), expected_output.splitlines())
        self.assertEqual(expected_command, result[0][0]["FailedCommand"])
        self.assertEqual(expected_info, result[0][0]["info"])
        self.assertEqual(expected_status, result[0][0]["status"])
        self.assertTrue(connected)

        # Cleanup
        db.disconnect()
