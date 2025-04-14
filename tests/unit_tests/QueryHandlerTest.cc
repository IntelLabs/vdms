/**
 * @file   QueryHandlerTest.cc
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2025 Intel Corporation
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

#include "gtest/gtest.h"

#include "QueryMessage.h"  // Protobuff implementation
#include "QueryHandlerBase.h"
#include "QueryHandlerPMGD.h"
#include "QueryHandlerNeo4j.h"
#include "VDMSConfig.h"

class QueryHandlerConcreteclass : public VDMS::QueryHandlerBase {
  public:
  QueryHandlerConcreteclass(){}
  ~QueryHandlerConcreteclass(){}

  void cleanup_query(const std::vector<std::string> &images,
                             const std::vector<std::string> &videos){
      VDMS::QueryHandlerBase::cleanup_query(images, videos);
  }

  void process_query(VDMS::protobufs::queryMessage &proto_query,
                     VDMS::protobufs::queryMessage &response){}
};

class QueryHandlerTest : public ::testing::Test {

protected:
  virtual void SetUp() {
    VDMS::VDMSConfig::init("unit_tests/config-pmgd-tests.json");
    VDMS::QueryHandlerPMGD::init();
   }
  virtual void TearDown() {
    VDMS::VDMSConfig::destroy();
  }
};

TEST_F(QueryHandlerTest, ProcessConnectionWithPMGD) {
    VDMS::QueryHandlerPMGD qh;
    comm::Connection *c = nullptr;
    ASSERT_ANY_THROW(qh.process_connection(c));
}

TEST_F(QueryHandlerTest, ProcessConnectionWithNeo4j) {
    VDMS::QueryHandlerNeo4j qh;
    comm::Connection *c = nullptr;
    ASSERT_ANY_THROW(qh.process_connection(c));
}

TEST_F(QueryHandlerTest, ProcessConnectionWithBaseClass) {
    QueryHandlerConcreteclass qh;
    comm::Connection *c = nullptr;
    ASSERT_ANY_THROW(qh.process_connection(c));
}

TEST_F(QueryHandlerTest, CleanupQueryInBaseClass) {
    QueryHandlerConcreteclass qh;
    std::vector<std::string> images;
    images.emplace_back("Invalid_image");
    std::vector<std::string> videos;
    ASSERT_NO_THROW( qh.cleanup_query(images, videos) );
}