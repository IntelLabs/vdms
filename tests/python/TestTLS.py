import ssl
import unittest
import vdms
import json
import os


class TestTLS(unittest.TestCase):
    untrusted_client_key = None
    untrusted_client_cert = None
    trusted_client_key = None
    trusted_client_cert = None
    trusted_server_key = None
    trusted_server_cert = None
    trusted_ca_cert = None
    props = {}
    addEntity = {}
    query = {}
    allQueries = []

    @classmethod
    def setUpClass(cls):
        cls.port = 55566
        cls.trusted_ca_cert = "/tmp/trusted_ca_cert.pem"
        cls.trusted_server_cert = "/tmp/trusted_server_cert.pem"
        cls.trusted_server_key = "/tmp/trusted_server_key.pem"
        cls.trusted_client_cert = "/tmp/trusted_client_cert.pem"
        cls.trusted_client_key = "/tmp/trusted_client_key.pem"
        cls.untrusted_client_cert = "/tmp/untrusted_client_cert.pem"
        cls.untrusted_client_key = "/tmp/untrusted_client_key.pem"

        cls.props = {}
        cls.props["place"] = "Mt Rainier"
        cls.props["id"] = 4543
        cls.props["type"] = "Volcano"

        cls.addEntity = {}
        cls.addEntity["properties"] = cls.props
        cls.addEntity["class"] = "Hike"

        cls.query = {}
        cls.query["AddEntity"] = cls.addEntity

        cls.allQueries = []
        cls.allQueries.append(cls.query)

    @classmethod
    def tearDownClass(cls):
        os.remove(cls.trusted_ca_cert)
        os.remove(cls.trusted_server_cert)
        os.remove(cls.trusted_server_key)
        os.remove(cls.trusted_client_cert)
        os.remove(cls.trusted_client_key)
        os.remove(cls.untrusted_client_cert)
        os.remove(cls.untrusted_client_key)
        os.remove("/tmp/trusted_ca_key.pem")
        os.remove("/tmp/untrusted_ca_cert.pem")
        os.remove("/tmp/untrusted_ca_key.pem")

    def test_fail_connect_without_cert(self):
        # Test without a cert, we still provide a ca cert to avoid failure because of server cert trust issues
        db = vdms.vdms(use_tls=True, ca_cert_file=self.trusted_ca_cert)
        with self.assertRaises(ssl.SSLError) as context:
            connected = db.connect("localhost", self.port)
            if connected:
                db.query(self.allQueries)
        self.assertIn("TLSV13_ALERT_CERTIFICATE_REQUIRED", str(context.exception))
        db.disconnect()

    def test_fail_with_bad_cert(self):
        db = vdms.vdms(
            use_tls=True,
            ca_cert_file=self.trusted_ca_cert,
            client_cert_file=self.untrusted_client_cert,
            client_key_file=self.untrusted_client_key,
        )
        with self.assertRaises(ssl.SSLError) as context:
            connected = db.connect("localhost", self.port)
            if connected:
                resp, res_arr = db.query(self.allQueries)
        self.assertIn("TLSV1_ALERT_DECRYPT_ERROR", str(context.exception))
        db.disconnect()

    def test_success_connect_with_cert(self):
        db = vdms.vdms(
            use_tls=True,
            ca_cert_file=self.trusted_ca_cert,
            client_cert_file=self.trusted_client_cert,
            client_key_file=self.trusted_client_key,
        )
        connected = db.connect("localhost", self.port)
        self.assertTrue(connected)

        resp, res_arr = db.query(self.allQueries)

        self.assertIsNotNone(resp)
        self.assertTrue(bool(resp))

        db.disconnect()
