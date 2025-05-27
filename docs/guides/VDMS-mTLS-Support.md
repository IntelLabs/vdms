# VDMS mTLS Support

By default, VDMS deploys in an insecure mode which allows the client to connect to the server without any authentication (user, password, or keys).
In some use-cases, a user may need some authentication to limit who can access the data in the VDMS server.
For such cases, VDMS has mutual TLS (mTLS) support to the server and client while retaining support for running in a TLS only (transport security only, no client authentication) or an insecure mode.

In mTLS, both client and server authenticate each other.
From the client's perspective, it verifies the server certificate's validity (i.e., it's not expired) and its endorsement by a trusted authority.
In the context of VDMS, this implies that we need to provide the client the CA certificate, which is the certificate authority responsible for creating the server's certificate.
This is done to ensure that, within the specific context of VDMS, it is a trusted certificate.
Conversely, the client presents its certificate to the server, which verifies its validity and its signing by the CA that the server uses for client authentication.
Hence, on the client side, the CA certificate establishes trust, while on the server side, it authenticates.
This is why both entities need to have the CA Certificate, as well as their respective certificates for identity verification.


## Generation of CA Certificates and keys
It's crucial to ensure that the CA certificate and key are present wherever the commands are executed.
If you decide to generate the client and server certificate and key separately at the client and the server, the CA certificate and key need to be present in both locations.
Please note that it should be the same CA certificate and key as both the server and client need to be signed by the same CA.

Below are steps to generate all CA certificates and keys.
In this case, these steps are performed on the system hosting the VDMS server.

1. ***Specify names for CA, Server and Client:*** Any value can be used for the CA certificate, but the client and server names should match the hostname or IP you use to connect. When testing with both client and server on the same system, you can simply use `localhost`. If you do not have DNS setup and prefer not to use an IP address, you can add entries to the hosts files, such as `client.vdms.local` and `server.vdms.local`, and use those in the certificates.  For the purpose of an example, we are assuming two systems are used: `systemA.domain` as the hostname hosting the VDMS server, and `systemB.domain` as the hostname hosting the VDMS client.<br>
    ```bash
    CA_NAME="ca.vdms.local"
    SERVER_NAME="systemA.domain"
    CLIENT_NAME="systemB.domain"
    ```

1. ***Specify validity and directory:*** Specify the length of certificate validity in days, and the directory to store the generated keys and certificates. In this example, the files are valid for 1 year (365 days) and the files are stored in directory `auth_files`.
    ```bash
    DAYS_VALID=365
    OUTPUT_DIR="<path_to_parent_key_directory>/auth_files"
    mkdir -p $OUTPUT_DIR
    ```

1. ***Generate CA private key:*** Here an RSA CA private key is generated with 2048 bits.
    ```bash
    openssl genrsa -out "${OUTPUT_DIR}/ca_key.pem" 2048
    ```

1. ***Generate CA self-signed certificate:*** Here a new self signed CA certificate is generated. In this example the country, state, city, and organization is used to set subject name.
    ```bash
    COUNTRY="<country>"
    STATE="<state>"
    CITY="<city>"
    ORG="<organization>"
    openssl req -x509 -new -nodes -key "${OUTPUT_DIR}/ca_key.pem" -sha256 -days $DAYS_VALID -out "${OUTPUT_DIR}/ca_cert.pem" -subj "/C=${COUNTRY}/ST=${STATE}/L=${CITY}/O=${ORG}/CN=${CA_NAME}"
    ```

1. ***Generate server private key:*** Here an RSA private key is generated for the server with 2048 bits.
    ```bash
    openssl genrsa -out "${OUTPUT_DIR}/server_key.pem" 2048
    ```

1. ***Generate server certificate signing request (CSR):*** Here a new self signed server certificate is generated.
    ```bash
    openssl req -new -key "${OUTPUT_DIR}/server_key.pem" -out "${OUTPUT_DIR}/server.csr" -subj "/C=${COUNTRY}/ST=${STATE}/L=${CITY}/O=${ORG}/CN=${SERVER_NAME}"
    ```

1. ***Sign the server CSR with CA certificate:*** Here the server CSR is signed with the CA certificate.
    ```bash
    openssl x509 -req -in "${OUTPUT_DIR}/server.csr" -CA "${OUTPUT_DIR}/ca_cert.pem" -CAkey "${OUTPUT_DIR}/ca_key.pem" -CAcreateserial -out "${OUTPUT_DIR}/server_cert.pem" -days $DAYS_VALID -sha256
    ```

1. ***Generate client private key:*** Here an RSA private key is generated for the client with 2048 bits.
    ```bash
    openssl genrsa -out "${OUTPUT_DIR}/client_key.pem" 2048
    ```

1. ***Generate client certificate signing request (CSR):*** Here a new self signed client certificate is generated.
    ```bash
    openssl req -new -key "${OUTPUT_DIR}/client_key.pem" -out "${OUTPUT_DIR}/client.csr" -subj "/C=${COUNTRY}/ST=${STATE}/L=${CITY}/O=${ORG}/CN=${CLIENT_NAME}"
    ```

1. ***Sign the client CSR with CA certificate:*** Here the client CSR is signed with the CA certificate.
    ```bash
    openssl x509 -req -in "${OUTPUT_DIR}/client.csr" -CA "${OUTPUT_DIR}/ca_cert.pem" -CAkey "${OUTPUT_DIR}/ca_key.pem" -CAcreateserial -out "${OUTPUT_DIR}/client_cert.pem" -days $DAYS_VALID -sha256
    ```

1. ***Cleanup CSR files:***
    ```bash
    rm "${OUTPUT_DIR}/server.csr"
    rm "${OUTPUT_DIR}/client.csr"
    ```
<br>

## Deploy VDMS using CA and Keys
To deploy VDMS with the proper credentials, we must update the `config-vdms.json`.
There are a few parameters needed to use this capability.

| Parameter   | Description                                 |
| ----------- | ------------------------------------------- |
| `ca_file`   | File for CA certificate                     |
| `cert_file` | File for Server certificate signed by CA    |
| `key_file`  | File for Server Certificate Signing Request |

If using the Docker Image, you can mount the directory containing the keys/certificates and also override the `config-vdms.json` parameters as follows:
```bash
docker run -d --net=host -v <path_to_parent_key_directory>/auth_files:/auth_files \
    -e OVERRIDE_cert_file=/auth_files/server_cert.pem \
    -e OVERRIDE_key_file=/auth_files/server_key.pem \
    -e OVERRIDE_ca_file=/auth_files/ca_cert.pem \
    intellabs/vdms:latest
```

If using a VDMS Server installed natively, simply update the `config-vdms.json` file prior to deploying the server.
<br>

## Client Authentication
To properly connect to the server, you must use the updated VDMS Client (v0.0.21+) and the CA certificate, client key, and client certificates must be on the system hosting the client.
There are a few additional parameters that should be provided to the client.

| Parameter        | Description                                                   |
| ---------------- | ------------------------------------------------------------- |
| `use_tls`          | Flag indicating whether to use TLS. Set this to True          |
| `ca_cert_file`     | Local path to the CA certificate used to sign certificates    |
| `client_cert_file` | Local path to the client CSR for secure communication         |
| `client_key_file`  | Local path to the client private key for secure communication |

With the mTLS support, there are a few limitations:

* If `ca_cert_file` is not set, client authentication will be disabled.
* Both `client_cert_file` and `client_key_file` must be specified together, not one and not the other.

Based on the generated files above, the files `ca_cert.pem`, `client_cert.pem`, and `client_key.pem` should be copied to a directory on the client system (hostname: `systemB.domain`).
For the sake of this example, let's assume these files were copied to `<local_path_to_parent_dir>/auth_files`.
To connect to the server on hostname `systemA.domain` using the default port (55555), use the following:
```python
import vdms
db = vdms.vdms(use_tls=True, ca_cert_file="<local_path_to_parent_dir>/auth_files/ca_cert.pem",
               client_cert_file="<local_path_to_parent_dir>/auth_files/client_cert.pem",
               client_key_file="<local_path_to_parent_dir>/auth_files/client_key.pem")
db.connect("systemA.domain", 55555)
```

Test change