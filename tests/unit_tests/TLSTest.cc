#include <string>
#include <thread>
#include <unistd.h>

#include "Connection.h"
#include "gtest/gtest.h"

#define SERVER_PORT_TLS 43445
#define NUMBER_OF_MESSAGES 1

typedef std::basic_string<uint8_t> BytesBuffer;

TEST(TLS_CPP, test_tls_server) {

  std::string client_to_server("client sends some random data");
  std::string server_to_client("this library seems to work :)");

  std::string cert_path = "/tmp/trusted_server_cert.pem";
  std::string key_path = "/tmp/trusted_server_key.pem";
  std::string ca_path = "/tmp/trusted_ca_cert.pem";

  std::string command = "cd tls_test && python3 prep-tls-tests.py > "
                        "../tests_tls_screen.log 2> ../tests_tls_log.log &";
  system(command.c_str());
  usleep(3 * 1000000);

  comm::ConnServer server(SERVER_PORT_TLS, cert_path, key_path, ca_path);
  comm::Connection conn_server(server.accept());

  for (int i = 0; i < NUMBER_OF_MESSAGES; ++i) {
    // Send something
    conn_server.send_message((const uint8_t *)server_to_client.c_str(),
                             server_to_client.length());
  }

  for (int i = 0; i < NUMBER_OF_MESSAGES; ++i) {
    // Receive something
    BytesBuffer message_received = conn_server.recv_message();
    std::string recv_message((char *)message_received.data());
    ASSERT_EQ(0, recv_message.compare(client_to_server));
  }
}
