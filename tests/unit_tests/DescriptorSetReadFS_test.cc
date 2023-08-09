/**
 * @file   DescriptorSetReadFS_test.cc
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2017 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files
 * (the "Software"), to deal
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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <list>

#include "helpers.h"
#include "vcl/VCL.h"
#include "gtest/gtest.h"

TEST(Descriptors_ReadFS, read_and_search_10k) {
  int nb = 10000;
  auto dimensions_list = get_dimensions_list();

  for (auto d : dimensions_list) {

    float *xb = generate_desc_linear_increase(d, nb);

    for (auto eng : get_engines()) {

      std::string index_filename = "dbs/read_and_search_10k" +
                                   std::to_string(d) + "_" +
                                   std::to_string(eng);
      {
        VCL::DescriptorSet index(index_filename, unsigned(d), eng);
        index.add(xb, nb);
        index.store();
      }

      VCL::DescriptorSet index_fs(index_filename);

      std::vector<float> distances;
      std::vector<long> desc_ids;
      index_fs.search(xb, 1, 4, desc_ids, distances);

      int exp = 0;
      for (auto &desc : desc_ids) {
        EXPECT_EQ(desc, exp++);
      }

      float results[] = {float(std::pow(0, 2) * d), float(std::pow(1, 2) * d),
                         float(std::pow(2, 2) * d), float(std::pow(3, 2) * d)};
      for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(distances[i], results[i]);
      }
    }

    delete[] xb;
  }
}

TEST(Descriptors_ReadFS, read_and_classify_10k) {
  int nb = 10000;

  auto dimensions_list = get_dimensions_list();

  for (auto d : dimensions_list) {

    float *xb = generate_desc_linear_increase(d, nb);

    for (auto eng : get_engines()) {
      std::string index_filename = "dbs/read_and_classify_10k" +
                                   std::to_string(d) + "_" +
                                   std::to_string(eng);
      int offset = 10;

      {
        VCL::DescriptorSet index(index_filename, unsigned(d), eng);

        std::vector<long> classes = classes_increasing_offset(nb, offset);

        index.add(xb, nb, classes);
        index.store();
      }

      VCL::DescriptorSet index_fs(index_filename);

      std::vector<long> ret_ids = index_fs.classify(xb, 60);

      int exp = 0;
      int i = 0;
      for (auto &id : ret_ids) {
        // printf("%ld - %ld \n", id, exp);
        EXPECT_EQ(id, exp);
        if (++i % offset == 0)
          ++exp;
      }
    }

    delete[] xb;
  }
}
