import socket
import ssl
import time


def print_and_flush(message):
    print(message, flush=True)


class TLSServer:
    def __init__(self, ca_cert_path, server_cert_path, server_key_path):
        self.ca_cert_path = ca_cert_path
        self.server_cert_path = server_cert_path
        self.server_key_path = server_key_path
        self.host = "localhost"
        self.port = 43446

    def serve(self):
        context = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)
        context.load_cert_chain(
            certfile=self.server_cert_path, keyfile=self.server_key_path
        )
        context.load_verify_locations(cafile=self.ca_cert_path)

        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            sock.bind((self.host, self.port))
            sock.listen(1)

            print_and_flush("Server is listening for connections...")

            loops = 1
            for _ in range(loops):
                conn, addr = sock.accept()
                with conn:
                    with context.wrap_socket(conn, server_side=True) as ssock:
                        print_and_flush(f"Connection established with {addr}")
                        self.handle_connection(ssock)
            print_and_flush("Server is done listening for connections.")
            return

    def handle_connection(self, ssock):
        try:
            # Read data from client, if any
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

            print_and_flush(f"Received from client: {buffer.decode()}")

            # Send response to the client
            msg = b"this library seems to work :)"
            send_size = len(msg)
            ssock.write(send_size.to_bytes(4, byteorder="little"))
            print_and_flush(f"Sent size: {send_size}")

            bytes_sent = 0
            while bytes_sent < send_size:
                sent = ssock.write(msg[bytes_sent:])
                print_and_flush(f"Sent {sent} bytes to the client.")
                if sent == 0:
                    print_and_flush("socket connection broken")
                    raise RuntimeError("socket connection broken")
                bytes_sent += sent
            print_and_flush("Sent response to the client.")
        except Exception as e:
            print_and_flush(f"Error during communication: {e}")


if __name__ == "__main__":

    tls_client = TLSServer(
        ca_cert_path="/tmp/trusted_ca_cert.pem",
        server_cert_path="/tmp/trusted_server_cert.pem",
        server_key_path="/tmp/trusted_server_key.pem",
    )
    tls_client.serve()
