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
