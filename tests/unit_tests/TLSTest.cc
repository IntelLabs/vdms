#include <string>
#include <thread>
#include <unistd.h>

#include "Connection.h"
#include "gtest/gtest.h"

#define SERVER_PORT_TLS 43445
#define PYTHON_SERVER_PORT_TLS 43446
#define NUMBER_OF_MESSAGES 1

typedef std::basic_string<uint8_t> BytesBuffer;

const std::string TMP_DIRNAME = "../tests_output_dir/";
const std::string TEMPORARY_DIR = "/tmp";

class TLS_CPP : public ::testing::Test {

protected:
  virtual void SetUp() {}
  virtual void TearDown() {}
};

TEST(TLS_CPP, test_tls_server) {

  std::string client_to_server("client sends some random data");
  std::string server_to_client("this library seems to work :)");

  std::string cert_path = TEMPORARY_DIR + "/trusted_server_cert.pem";
  std::string key_path = TEMPORARY_DIR + "/trusted_server_key.pem";
  std::string ca_path = TEMPORARY_DIR + "/trusted_ca_cert.pem";

  std::string command = "cd tls_test && python3 run_tls_test_server.py > " +
                        TMP_DIRNAME + "tests_tls_server_screen.log 2> " +
                        TMP_DIRNAME + "tests_tls_server_log.log &";
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

TEST(TLS_CPP, test_tls_client) {

  std::string client_to_server("client sends some random data");
  std::string server_to_client("this library seems to work :)");

  std::string cert_path = TEMPORARY_DIR + "/trusted_client_cert.pem";
  std::string key_path = TEMPORARY_DIR + "/trusted_client_key.pem";
  std::string ca_path = TEMPORARY_DIR + "/trusted_ca_cert.pem";

  std::string command = "cd tls_test && python3 run_tls_test_client.py > " +
                        TMP_DIRNAME + "tests_tls_client_screen.log 2> " +
                        TMP_DIRNAME + "tests_tls_client_log.log &";
  system(command.c_str());
  usleep(3 * 1000000);

  comm::ConnClient client("localhost", PYTHON_SERVER_PORT_TLS, cert_path,
                          key_path, ca_path);

  for (int i = 0; i < NUMBER_OF_MESSAGES; ++i) {
    client.send_message((const uint8_t *)client_to_server.c_str(),
                        client_to_server.length());
  }

  for (int i = 0; i < NUMBER_OF_MESSAGES; ++i) {
    BytesBuffer message_received = client.recv_message();
    std::string recv_message((char *)message_received.data());
    ASSERT_EQ(0, recv_message.compare(server_to_client));
  }
}