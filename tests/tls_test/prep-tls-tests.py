from cryptography import x509
from cryptography.x509.oid import NameOID
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.hazmat.primitives.serialization import (
    Encoding,
    PrivateFormat,
    BestAvailableEncryption,
    NoEncryption,
)
from cryptography.hazmat.backends import default_backend
import datetime
import os
import socket
import ssl
import time


def print_and_flush(message):
    print(message, flush=True)


class TLSClient:
    def __init__(self, ca_cert_path, client_cert_path, client_key_path, timeout=1800):
        self.ca_cert_path = ca_cert_path
        self.client_cert_path = client_cert_path
        self.client_key_path = client_key_path
        self.timeout = timeout
        self.host = "localhost"
        self.port = 43445

    def create_connection(self):
        context = ssl.create_default_context(
            ssl.Purpose.SERVER_AUTH, cafile=self.ca_cert_path
        )
        context.load_cert_chain(
            certfile=self.client_cert_path, keyfile=self.client_key_path
        )

        start_time = time.time()
        end_time = start_time + self.timeout

        while time.time() < end_time:
            try:
                with socket.create_connection(
                    (self.host, self.port), timeout=self.timeout
                ) as sock:
                    with context.wrap_socket(sock, server_hostname=self.host) as ssock:
                        print_and_flush("Connection established.")
                        self.handle_connection(ssock)
                        return
            except (ConnectionRefusedError, socket.timeout) as e:
                time.sleep(
                    0.1
                )  # wait a bit before retrying to avoid flooding with attempts

        elapsed_time = time.time() - start_time
        print(
            f"Connection attempts failed. Timed out after {elapsed_time:.2f} seconds."
        )

    def handle_connection(self, ssock):
        try:
            # Read data from server, if any
            size_data = ssock.read(4)
            recv_size = int.from_bytes(size_data, byteorder="little")
            print_and_flush(f"Received size: {recv_size}")

            buffer = b""
            while len(buffer) < recv_size:
                data = ssock.read(1024)
                print_and_flush(f"Received data: {data}")
                if data == "":
                    print_and_flush("socket connection broken")
                    break
                buffer += data

            print_and_flush(f"Received from server: {buffer.decode()}")

            # Send response to the server
            msg = b"client sends some random data"
            send_size = len(msg)
            ssock.write(send_size.to_bytes(4, byteorder="little"))
            print_and_flush(f"Sent size: {send_size}")

            bytes_sent = 0
            while bytes_sent < send_size:
                sent = ssock.write(msg[bytes_sent:])
                print_and_flush(f"Sent {sent} bytes to the server.")
                if sent == 0:
                    print_and_flush("socket connection broken")
                    raise RuntimeError("socket connection broken")
                bytes_sent += sent
            print_and_flush("Sent response to the server.")
        except Exception as e:
            print_and_flush(f"Error during communication: {e}")


def generate_private_key():
    return rsa.generate_private_key(
        public_exponent=65537, key_size=2048, backend=default_backend()
    )


def generate_ca_certificate(subject_name, private_key):
    subject = issuer = x509.Name(
        [
            x509.NameAttribute(NameOID.COUNTRY_NAME, "US"),
            x509.NameAttribute(NameOID.STATE_OR_PROVINCE_NAME, "Oregon"),
            x509.NameAttribute(NameOID.LOCALITY_NAME, "Hillsboro"),
            x509.NameAttribute(NameOID.ORGANIZATION_NAME, "Intel Corporation"),
            x509.NameAttribute(NameOID.COMMON_NAME, subject_name),
        ]
    )

    certificate = (
        x509.CertificateBuilder()
        .subject_name(subject)
        .issuer_name(issuer)
        .public_key(private_key.public_key())
        .serial_number(x509.random_serial_number())
        .not_valid_before(datetime.datetime.utcnow())
        .not_valid_after(
            # Our certificate will be valid for 10 days
            datetime.datetime.utcnow()
            + datetime.timedelta(days=10)
        )
        .add_extension(
            x509.BasicConstraints(ca=True, path_length=None),
            critical=True,
        )
        .sign(private_key, hashes.SHA256(), default_backend())
    )

    return certificate


def generate_signed_certificate(
    subject_name, issuer_certificate, issuer_private_key, subject_private_key
):
    subject = x509.Name(
        [
            x509.NameAttribute(NameOID.COUNTRY_NAME, "US"),
            x509.NameAttribute(NameOID.STATE_OR_PROVINCE_NAME, "Oregon"),
            x509.NameAttribute(NameOID.LOCALITY_NAME, "Hillsboro"),
            x509.NameAttribute(NameOID.ORGANIZATION_NAME, "Intel Corporation"),
            x509.NameAttribute(NameOID.COMMON_NAME, subject_name),
        ]
    )

    issuer = issuer_certificate.subject

    certificate = (
        x509.CertificateBuilder()
        .subject_name(subject)
        .issuer_name(issuer)
        .public_key(subject_private_key.public_key())
        .serial_number(x509.random_serial_number())
        .not_valid_before(datetime.datetime.utcnow())
        .not_valid_after(
            # Our certificate will be valid for 10 days
            datetime.datetime.utcnow()
            + datetime.timedelta(days=10)
        )
        .add_extension(
            x509.BasicConstraints(ca=False, path_length=None),
            critical=True,
        )
        .add_extension(
            x509.SubjectAlternativeName([x509.DNSName(subject_name)]),
            critical=False,
        )
        .sign(issuer_private_key, hashes.SHA256(), default_backend())
    )

    return certificate


def write_to_disk(directory, name, key, cert):
    with open(os.path.join(directory, f"{name}_key.pem"), "wb") as f:
        f.write(key.private_bytes(Encoding.PEM, PrivateFormat.PKCS8, NoEncryption()))

    with open(os.path.join(directory, f"{name}_cert.pem"), "wb") as f:
        f.write(cert.public_bytes(Encoding.PEM))


if __name__ == "__main__":

    #####################################################################################
    # GENERATE TRUSTED CERTS AND KEYS
    #####################################################################################
    # Generate CA key and certificate
    trusted_ca_key = generate_private_key()
    trusted_ca_cert = generate_ca_certificate("ca.vdms.local", trusted_ca_key)

    # Generate server key and certificate signed by CA
    server_key = generate_private_key()
    server_cert = generate_signed_certificate(
        "localhost", trusted_ca_cert, trusted_ca_key, server_key
    )

    # Generate client key and certificate signed by CA
    trusted_client_key = generate_private_key()
    trusted_client_cert = generate_signed_certificate(
        "client.vdms.local", trusted_ca_cert, trusted_ca_key, trusted_client_key
    )

    # Write keys and certificates to disk
    write_to_disk("/tmp", "trusted_ca", trusted_ca_key, trusted_ca_cert)
    write_to_disk("/tmp", "trusted_server", server_key, server_cert)
    write_to_disk("/tmp", "trusted_client", trusted_client_key, trusted_client_cert)

    #####################################################################################
    # GENERATE UNTRUSTED CERTS AND KEYS TO ENSURE UNTRUSTED CLIENT CERTS AREN'T ACCEPTED
    #####################################################################################
    # Generate CA key and certificate
    untrusted_ca_key = generate_private_key()
    untrusted_ca_cert = generate_ca_certificate("ca.vdms.local", untrusted_ca_key)

    # Generate client key and certificate signed by CA
    untrusted_client_key = generate_private_key()
    untrusted_client_cert = generate_signed_certificate(
        "client.vdms.local", untrusted_ca_cert, untrusted_ca_key, untrusted_client_key
    )

    # Write keys and certificates to disk
    write_to_disk("/tmp", "untrusted_ca", untrusted_ca_key, untrusted_ca_cert)
    write_to_disk(
        "/tmp", "untrusted_client", untrusted_client_key, untrusted_client_cert
    )

    tls_client = TLSClient(
        ca_cert_path="/tmp/trusted_ca_cert.pem",
        client_cert_path="/tmp/trusted_client_cert.pem",
        client_key_path="/tmp/trusted_client_key.pem",
        timeout=1800,
    )
    tls_client.create_connection()
