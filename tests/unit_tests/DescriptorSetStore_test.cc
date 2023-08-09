/**
 * @file   DescriptorSetStore_test.cc
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

#include "helpers.h"
#include "vcl/VCL.h"
#include "gtest/gtest.h"

TEST(Descriptors_Store, add_ivfflatl2_100d_2add_file) {
  int d = 100;
  int nb = 10000;
  float *xb = generate_desc_linear_increase(d, nb);

  std::string index_filename = "dbs/store_ivfflatl2_100d_2add.faiss";
  VCL::DescriptorSet index(index_filename, unsigned(d), VCL::FaissIVFFlat);

  index.add(xb, nb);
  index.store();

  generate_desc_linear_increase(d, nb, xb, .6);

  VCL::DescriptorSet index_f(index_filename);
  index_f.add(xb, nb);

  generate_desc_linear_increase(d, 4, xb, 0);

  std::vector<float> distances;
  std::vector<long> desc_ids;
  index_f.search(xb, 1, 4, desc_ids, distances);

  float results[] = {0, 36, 100, 256};
  for (int i = 0; i < 4; ++i) {
    EXPECT_EQ(std::round(distances[i]), std::round(results[i]));
  }

  index_f.store();

  delete[] xb;
}

TEST(Descriptors_Store, add_tiledbdense_100d_file) {
  int d = 100;
  int nb = 10000;
  float *xb = generate_desc_linear_increase(d, nb);

  std::string index_filename = "dbs/store_tiledbdense_100d_tdb";
  VCL::DescriptorSet index_f(index_filename, unsigned(d), VCL::TileDBDense);

  index_f.add(xb, nb);
  index_f.store();

  VCL::DescriptorSet index(index_filename);

  std::vector<float> distances;
  std::vector<long> desc_ids;
  index.search(xb, 1, 4, desc_ids, distances);

  int exp = 0;
  for (auto &desc : desc_ids) {
    EXPECT_EQ(desc, exp++);
  }

  int results[] = {0, 100, 400, 900};
  for (int i = 0; i < 4; ++i) {
    EXPECT_EQ(distances[i], results[i]);
  }

  index.store();

  delete[] xb;
}

TEST(Descriptors_Store, add_tiledbdense_100d_2add_file) {
  int d = 100;
  int nb = 10000;
  float *xb = generate_desc_linear_increase(d, nb);

  std::string index_filename = "dbs/store_tiledbdense_100d_2add";
  VCL::DescriptorSet index_f(index_filename, unsigned(d), VCL::TileDBDense);

  index_f.add(xb, nb);

  generate_desc_linear_increase(d, nb, xb, .6);

  index_f.add(xb, nb);
  index_f.store();

  generate_desc_linear_increase(d, 4, xb, 0);

  VCL::DescriptorSet index(index_filename);

  std::vector<float> distances;
  std::vector<long> desc_ids;
  index.search(xb, 1, 4, desc_ids, distances);

  float results[] = {0, 36, 100, 256};
  // This is:
  //  (0)  ^2 * 100 = 0
  //  (0.6)^2 * 100 = 36
  //  (1  )^2 * 100 = 100
  //  (1.6)^2 * 100 = 256

  for (int i = 0; i < 4; ++i) {
    EXPECT_EQ(std::round(distances[i]), std::round(results[i]));
  }

  index.store();
  delete[] xb;
}
