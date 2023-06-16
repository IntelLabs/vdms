/**
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2017 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <string>
#include <thread>

#include "Connection.h"
#include "gtest/gtest.h"

#define SERVER_PORT_INTERCHANGE 43444
#define SERVER_PORT_MULTIPLE 43444
#define NUMBER_OF_MESSAGES 20

typedef std::basic_string<uint8_t> BytesBuffer;

// Ping-pong messages between server and client
TEST(CommTest, SyncMessages) {
  std::string client_to_server("testing this awesome comm library with "
                               "come random data");
  std::string server_to_client("this awesome library seems to work :)");

  std::thread server_thread([client_to_server, server_to_client]() {
    comm::ConnServer server(SERVER_PORT_INTERCHANGE);
    comm::Connection conn_server(server.accept());

    for (int i = 0; i < NUMBER_OF_MESSAGES; ++i) {
      // Recieve something
      BytesBuffer message_received = conn_server.recv_message();
      std::string recv_message((char *)message_received.data());
      ASSERT_EQ(0, recv_message.compare(client_to_server));

      // Send something
      conn_server.send_message((const uint8_t *)server_to_client.c_str(),
                               server_to_client.length());
    }
  });

  server_thread.detach();

  comm::ConnClient conn_client("localhost", SERVER_PORT_INTERCHANGE);

  for (int i = 0; i < NUMBER_OF_MESSAGES; ++i) {
    // Send something
    conn_client.send_message((const uint8_t *)client_to_server.c_str(),
                             client_to_server.length());

    // Receive something
    BytesBuffer message_received = conn_client.recv_message();
    std::string recv_message((char *)message_received.data());
    ASSERT_EQ(0, recv_message.compare(server_to_client));
  }
}

// Both client and server send all messages firsts and then check the received
// messages.
TEST(CommTest, AsyncMessages) {
  std::string client_to_server("client sends some random data");
  std::string server_to_client("this library seems to work :)");

  std::thread server_thread([client_to_server, server_to_client]() {
    comm::ConnServer server(SERVER_PORT_MULTIPLE);
    comm::Connection conn_server(server.accept());

    for (int i = 0; i < NUMBER_OF_MESSAGES; ++i) {
      // Send something
      conn_server.send_message((const uint8_t *)server_to_client.c_str(),
                               server_to_client.length());
    }

    for (int i = 0; i < NUMBER_OF_MESSAGES; ++i) {
      // Recieve something
      BytesBuffer message_received = conn_server.recv_message();
      std::string recv_message((char *)message_received.data());
      ASSERT_EQ(0, recv_message.compare(client_to_server));
    }
  });
  server_thread.detach();

  comm::ConnClient conn_client("localhost", SERVER_PORT_MULTIPLE);

  for (int i = 0; i < NUMBER_OF_MESSAGES; ++i) {
    // Send something
    conn_client.send_message((const uint8_t *)(client_to_server).c_str(),
                             (client_to_server).length());
  }

  for (int i = 0; i < NUMBER_OF_MESSAGES; ++i) {

    // Receive something
    BytesBuffer message_received = conn_client.recv_message();
    std::string recv_message((char *)message_received.data());
    ASSERT_EQ(0, recv_message.compare(server_to_client));
  }
}

// Server accepts connection and then goes down, client tries to send.
TEST(CommTest, ServerShutdownSend) {
  std::string client_to_server("testing this awesome comm library "
                               "with some random data");
  std::string server_to_client("this awesome library seems to work :)");

  std::thread server_thread([client_to_server, server_to_client]() {
    comm::ConnServer server(SERVER_PORT_INTERCHANGE);
    comm::Connection conn_server(server.accept());
  });

  comm::ConnClient conn_client("localhost", SERVER_PORT_INTERCHANGE);

  server_thread.join(); // Here the server will close the port.

  ASSERT_THROW(
      conn_client.send_message((const uint8_t *)client_to_server.c_str(),
                               client_to_server.length()),
      comm::ExceptionComm);
}

// Server accepts connection and then goes down, client tries to recv.
TEST(CommTest, ServerShutdownRecv) {
  std::string client_to_server("testing this awesome comm "
                               "library with some random data");

  std::thread server_thread([client_to_server]() {
    comm::ConnServer server(SERVER_PORT_INTERCHANGE);
    comm::Connection conn_server(server.accept());
  });

  comm::ConnClient conn_client("localhost", SERVER_PORT_INTERCHANGE);

  server_thread.join(); // Here the server will close the port.

  ASSERT_THROW(BytesBuffer message_received = conn_client.recv_message(),
               comm::ExceptionComm);
}

TEST(CommTest, SendArrayInts) {
  int arr[10] = {22, 568, 254, 784, 452, 458, 235, 124, 1425, 1542};
  std::thread server_thread([arr]() {
    comm::ConnServer server(SERVER_PORT_INTERCHANGE);
    comm::Connection conn_server(server.accept());

    conn_server.send_message((uint8_t *)arr, sizeof(arr));
  });

  server_thread.detach();

  comm::ConnClient conn_client("localhost", SERVER_PORT_INTERCHANGE);
  BytesBuffer message_received = conn_client.recv_message();

  int *arr_recv = (int *)message_received.data();
  for (int i = 0; i < 10; ++i) {
    ASSERT_EQ(arr[i], arr_recv[i]);
  }
}

TEST(CommTest, MoveCopy) {
  comm::Connection a;
  comm::Connection conn_server;
  conn_server = std::move(a); // Testing copy with move works
}

TEST(CommTest, Unreachable) {
  ASSERT_THROW(
      comm::ConnClient conn_client("unreachable.com.ar.something", 5555),
      comm::ExceptionComm);

  ASSERT_THROW(comm::ConnClient conn_client("localhost", -1),
               comm::ExceptionComm);
}

TEST(CommTest, ServerWrongPort) {
  ASSERT_THROW(comm::ConnServer conn_server(-22), comm::ExceptionComm);

  ASSERT_THROW(comm::ConnServer conn_server(0), comm::ExceptionComm);
}

TEST(CommTest, ClientWrongAddrOrPort) {
  ASSERT_THROW(comm::ConnClient conn_client("", 3424), comm::ExceptionComm);

  ASSERT_THROW(comm::ConnClient conn_client("intel.com", -32),
               comm::ExceptionComm);

  ASSERT_THROW(comm::ConnClient conn_client("intel.com", 0),
               comm::ExceptionComm);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);

  // To make GoogleTest silent:
  // if (true) {
  //     auto& listeners = ::testing::UnitTest::GetInstance()->listeners();
  //     delete listeners.Release(listeners.default_result_printer());
  // }
  return RUN_ALL_TESTS();
}
