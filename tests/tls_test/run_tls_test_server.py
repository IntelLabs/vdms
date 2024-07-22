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


if __name__ == "__main__":

    tls_client = TLSClient(
        ca_cert_path="/tmp/trusted_ca_cert.pem",
        client_cert_path="/tmp/trusted_client_cert.pem",
        client_key_path="/tmp/trusted_client_key.pem",
        timeout=1800,
    )
    tls_client.create_connection()
