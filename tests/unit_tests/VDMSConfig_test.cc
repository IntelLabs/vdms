/**
 * @file   VDMSConfig_test.cc
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2024 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */
#include <exception>
#include <stdio.h>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "VDMSConfig.h"

using ::testing::_;
using ::testing::HasSubstr;
using ::testing::Return;
using ::testing::ThrowsMessage;

using namespace VDMS;

class VDMSConfigTest : public ::testing::Test {

protected:
  virtual void SetUp() {
    // In case of another test suit created an instance of VDMSConfig
    // then VDMSConfigTest has to destroy it before running the tests
    VDMSConfig *instance = VDMSConfig::instance();
    if (instance) {
      bool result = VDMSConfig::destroy();
      if (result) {
        std::cout << "It looks like another test left by mistake "
                  << "an instance of VDMSConfig running.\nHowever, "
                  << "VDMSConfigTest::SetUp() just destroyed it." << std::endl;
      }
    }
  }
  virtual void TearDown() {
    VDMSConfig *instance = VDMSConfig::instance();
    if (instance) {
      bool result = VDMSConfig::destroy();
      if (result) {
        std::cout << "Warning: Please check the tests written for VDMSConfig."
                  << "\nIt looks like one of them is not destroying"
                  << " correctly the instance of VDMSConfig." << std::endl;
      }
    }
  }
};

TEST_F(VDMSConfigTest, init_TRUE_TEST) {
  try {
    // Setup
    std::string config_file = "unit_tests/config-for-vdms-config-v1-tests.json";

    // Execute the test
    bool result = VDMSConfig::init(config_file);

    // Verify the results
    EXPECT_TRUE(result);
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }

  VDMSConfig::destroy();
}

TEST_F(VDMSConfigTest, init_FALSE_TEST) {
  try {
    // Setup
    std::string config_file = "unit_tests/config-for-vdms-config-v1-tests.json";
    bool result_true = VDMSConfig::init(config_file);
    EXPECT_TRUE(result_true);

    // Execute the test
    bool result_false = VDMSConfig::init(config_file);

    // Verify the results
    EXPECT_FALSE(result_false);
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }

  VDMSConfig::destroy();
}

TEST_F(VDMSConfigTest, instance_NON_NULL_TEST) {
  try {
    // Setup
    std::string config_file = "unit_tests/config-for-vdms-config-v1-tests.json";
    bool result = VDMSConfig::init(config_file);
    EXPECT_TRUE(result);

    // Execute the test
    VDMSConfig *instance = VDMSConfig::instance();

    // Verify the results
    EXPECT_TRUE(nullptr != instance);
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }

  VDMSConfig::destroy();
}

TEST_F(VDMSConfigTest, instance_WITH_NULL_TEST) {
  try {
    // Execute the test
    VDMSConfig *instance = VDMSConfig::instance();

    // Verify the results
    EXPECT_EQ(instance, nullptr);
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(VDMSConfigTest, destroy_NON_NULL_TEST) {
  try {
    // Setup
    std::string config_file = "unit_tests/config-for-vdms-config-v1-tests.json";
    bool result1 = VDMSConfig::init(config_file);
    EXPECT_TRUE(result1);

    // Execute the test
    bool result2 = VDMSConfig::destroy();

    // Verify the results
    EXPECT_TRUE(result2);
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(VDMSConfigTest, destroy_WITH_NULL_TEST) {
  try {
    // Execute the test
    bool result = VDMSConfig::destroy();

    // Verify the results
    EXPECT_FALSE(result);
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(VDMSConfigTest, get_endpoint_override_TEST) {
  try {
    // Setup
    bool initialized =
        VDMSConfig::init("unit_tests/config-for-vdms-config-v1-tests.json");
    EXPECT_TRUE(initialized);

    VDMSConfig *config = VDMSConfig::instance();
    std::string expected_endpoint_override = "http://127.0.0.2:9000";

    // Execute the test
    std::optional<std::string> returned_endpoint_override_ptr =
        config->get_endpoint_override();

    // Verify the results
    EXPECT_TRUE(returned_endpoint_override_ptr != std::nullopt);
    std::string returned_endpoint_override_str =
        *returned_endpoint_override_ptr;
    EXPECT_STREQ(expected_endpoint_override.c_str(),
                 returned_endpoint_override_str.c_str());
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
  VDMSConfig::destroy();
}

TEST_F(VDMSConfigTest, get_endpoint_override_WITH_DEFAULT_TEST) {
  try {
    // Setup
    bool initialized =
        VDMSConfig::init("unit_tests/config-for-vdms-config-v2-tests.json");
    EXPECT_TRUE(initialized);

    VDMSConfig *config = VDMSConfig::instance();
    std::string expected_endpoint_override = "http://127.0.0.1:9000";

    // Execute the test
    std::optional<std::string> returned_endpoint_override_ptr =
        config->get_endpoint_override();

    // Verify the results
    EXPECT_TRUE(returned_endpoint_override_ptr != std::nullopt);
    std::string returned_endpoint_override_str =
        *returned_endpoint_override_ptr;
    EXPECT_STREQ(expected_endpoint_override.c_str(),
                 returned_endpoint_override_str.c_str());
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
  VDMSConfig::destroy();
}

TEST_F(VDMSConfigTest, get_proxy_host_TEST) {
  try {
    // Setup
    bool initialized =
        VDMSConfig::init("unit_tests/config-for-vdms-config-v1-tests.json");
    EXPECT_TRUE(initialized);

    VDMSConfig *config = VDMSConfig::instance();
    std::string expected_proxy_host = "a.proxy.to.be.tested.intel.com";

    // Execute the test
    std::optional<std::string> returned_proxy_host_ptr =
        config->get_proxy_host();

    // Verify the results
    EXPECT_TRUE(returned_proxy_host_ptr != std::nullopt);
    std::string returned_proxy_host_str = *returned_proxy_host_ptr;
    EXPECT_STREQ(expected_proxy_host.c_str(), returned_proxy_host_str.c_str());
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
  VDMSConfig::destroy();
}

TEST_F(VDMSConfigTest, get_proxy_host_WITH_MISSING_CONFIG_TEST) {
  try {
    // Setup
    bool initialized =
        VDMSConfig::init("unit_tests/config-for-vdms-config-v2-tests.json");
    EXPECT_TRUE(initialized);

    VDMSConfig *config = VDMSConfig::instance();

    // Execute the test
    std::optional<std::string> returned_proxy_host_ptr =
        config->get_proxy_host();

    // Verify the results
    EXPECT_TRUE(returned_proxy_host_ptr == std::nullopt);
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
  VDMSConfig::destroy();
}

TEST_F(VDMSConfigTest, get_proxy_port_TEST) {
  try {
    // Setup
    bool initialized =
        VDMSConfig::init("unit_tests/config-for-vdms-config-v1-tests.json");
    EXPECT_TRUE(initialized);

    VDMSConfig *config = VDMSConfig::instance();
    int expected_proxy_port = 8000;

    // Execute the test
    std::optional<int> returned_proxy_port_ptr = config->get_proxy_port();

    // Verify the results
    EXPECT_TRUE(returned_proxy_port_ptr != std::nullopt);
    int returned_proxy_port = *returned_proxy_port_ptr;
    EXPECT_EQ(expected_proxy_port, returned_proxy_port);
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
  VDMSConfig::destroy();
}

TEST_F(VDMSConfigTest, get_proxy_port_WITH_MISSING_CONFIG_TEST) {
  try {
    // Setup
    bool initialized =
        VDMSConfig::init("unit_tests/config-for-vdms-config-v2-tests.json");
    EXPECT_TRUE(initialized);

    VDMSConfig *config = VDMSConfig::instance();

    // Execute the test
    std::optional<int> returned_proxy_port_ptr = config->get_proxy_port();

    // Verify the results
    EXPECT_TRUE(returned_proxy_port_ptr == std::nullopt);
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
  VDMSConfig::destroy();
}

TEST_F(VDMSConfigTest, get_proxy_scheme_TEST) {
  try {
    // Setup
    bool initialized =
        VDMSConfig::init("unit_tests/config-for-vdms-config-v1-tests.json");
    EXPECT_TRUE(initialized);

    VDMSConfig *config = VDMSConfig::instance();
    std::string expected_proxy_scheme = "http";

    // Execute the test
    std::optional<std::string> returned_proxy_scheme_ptr =
        config->get_proxy_scheme();

    // Verify the results
    EXPECT_TRUE(returned_proxy_scheme_ptr != std::nullopt);
    std::string returned_proxy_scheme_str = *returned_proxy_scheme_ptr;
    EXPECT_STREQ(expected_proxy_scheme.c_str(),
                 returned_proxy_scheme_str.c_str());
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
  VDMSConfig::destroy();
}

TEST_F(VDMSConfigTest, get_proxy_scheme_WITH_MISSING_CONFIG_TEST) {
  try {
    // Setup
    bool initialized =
        VDMSConfig::init("unit_tests/config-for-vdms-config-v2-tests.json");
    EXPECT_TRUE(initialized);

    VDMSConfig *config = VDMSConfig::instance();

    // Execute the test
    std::optional<std::string> returned_proxy_scheme_ptr =
        config->get_proxy_scheme();

    // Verify the results
    EXPECT_TRUE(returned_proxy_scheme_ptr == std::nullopt);
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
  VDMSConfig::destroy();
}

TEST_F(VDMSConfigTest, get_aws_flag_TRUE_TEST) {
  try {
    // Setup
    bool initialized =
        VDMSConfig::init("unit_tests/config-for-vdms-config-v1-tests.json");
    EXPECT_TRUE(initialized);
    VDMSConfig *config = VDMSConfig::instance();

    // Execute the test
    bool returned_aws_flag = config->get_aws_flag();

    // Verify the results
    EXPECT_TRUE(returned_aws_flag);
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
  VDMSConfig::destroy();
}

TEST_F(VDMSConfigTest, get_aws_flag_FALSE_TEST) {
  try {
    // Setup
    bool initialized =
        VDMSConfig::init("unit_tests/config-for-vdms-config-v3-tests.json");
    EXPECT_TRUE(initialized);
    VDMSConfig *config = VDMSConfig::instance();

    // Execute the test
    bool returned_aws_flag = config->get_aws_flag();

    // Verify the results
    EXPECT_FALSE(returned_aws_flag);
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
  VDMSConfig::destroy();
}