/**
 * @file   DescriptorSetTrain_test.cc
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
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>

#include "helpers.h"
#include "vcl/VCL.h"
#include "gtest/gtest.h"

TEST(Descriptors_Train, train_flatl2_4d) {
  int d = 4;
  int nb = 10000;

  float *xb = generate_desc_linear_increase(d, nb);

  std::string index_filename = "dbs/train_flatl2_4d.faiss";
  VCL::DescriptorSet index(index_filename, unsigned(d), VCL::FaissFlat);

  int offset = 10;
  std::vector<long> classes = classes_increasing_offset(nb, offset);

  index.add(xb, nb, classes);

  index.train();

  std::vector<float> distances;
  std::vector<long> desc_ids;
  index.search(xb, 1, 4, desc_ids, distances);

  int exp = 0;
  for (auto &desc : desc_ids) {
    EXPECT_EQ(desc, exp++);
  }

  int results[] = {0, 4, 16, 36};
  for (int i = 0; i < 4; ++i) {
    EXPECT_EQ(distances[i], results[i]);
  }

  std::vector<long> ret_ids = index.classify(xb, 60);

  exp = 0;
  int i = 0;
  for (auto &id : ret_ids) {
    EXPECT_EQ(id, exp);
    if (++i % offset == 0)
      ++exp;
  }

  index.store();

  delete[] xb;
}

TEST(Descriptors_Train, train_10k) {
  int nb = 10000;

  auto dimensions_list = get_dimensions_list();

  for (auto d : dimensions_list) {

    float *xb = generate_desc_linear_increase(d, nb);

    for (auto eng : get_engines()) {
      std::string index_filename =
          "dbs/train_10k" + std::to_string(d) + "_" + std::to_string(eng);

      VCL::DescriptorSet index(index_filename, unsigned(d), eng);

      int offset = 10;
      std::vector<long> classes = classes_increasing_offset(nb, offset);

      index.add(xb, nb, classes);

      index.train();

      std::vector<float> distances;
      std::vector<long> desc_ids;
      index.search(xb, 1, 4, desc_ids, distances);

      int exp = 0;
      for (auto &desc : desc_ids) {
        EXPECT_EQ(desc, exp++);
      }

      std::vector<long> ret_ids = index.classify(xb, 60);

      exp = 0;
      int i = 0;
      for (auto &id : ret_ids) {
        // printf("%ld - %ld \n", id, exp);
        EXPECT_EQ(id, exp);
        if (++i % offset == 0)
          ++exp;
      }

      index.store();
    }

    delete[] xb;
  }
}

// String labels tests

TEST(Descriptors_Train, train_ivfflatl2_4d_labels) {
  int d = 4;
  int nb = 10000;

  float *xb = generate_desc_linear_increase(d, nb);

  auto class_map = animals_map();

  std::string index_filename = "dbs/train_ivfflatl2_4d_labels.faiss";
  VCL::DescriptorSet index(index_filename, unsigned(d), VCL::FaissIVFFlat);

  int offset = 10;
  std::vector<long> classes = classes_increasing_offset(nb, offset);

  index.set_labels_map(class_map);

  index.add(xb, nb, classes);

  index.train();

  std::vector<float> distances;
  std::vector<long> desc_ids;
  index.search(xb, 1, 4, desc_ids, distances);

  int exp = 0;
  for (auto &desc : desc_ids) {
    EXPECT_EQ(desc, exp++);
  }

  int results[] = {0, 4, 16, 36};
  for (int i = 0; i < 4; ++i) {
    EXPECT_EQ(distances[i], results[i]);
  }

  std::vector<long> ret_ids = index.classify(xb, 60);
  std::vector<std::string> ret = index.label_id_to_string(ret_ids);

  for (int i = 0; i < offset; ++i) {
    EXPECT_EQ(ret[i], "parrot");
    EXPECT_EQ(ret[i + offset], "dog");
    EXPECT_EQ(ret[i + 2 * offset], "cat");
    EXPECT_EQ(ret[i + 3 * offset], "messi");
    EXPECT_EQ(ret[i + 4 * offset], "bird");
    EXPECT_EQ(ret[i + 5 * offset], "condor");
  }

  index.search(xb, 1, offset, desc_ids, distances);
  ret = index.get_str_labels(desc_ids);

  for (auto &label : ret) {
    EXPECT_EQ(label, "parrot");
  }

  delete[] xb;
}

TEST(Descriptors_Train, train_labels_10k) {
  int nb = 10000;

  auto dimensions_list = get_dimensions_list();
  auto class_map = animals_map();

  for (auto d : dimensions_list) {
    float *xb = generate_desc_linear_increase(d, nb);

    for (auto eng : get_engines()) {
      std::string index_filename = "dbs/train_labels_10k_" + std::to_string(d) +
                                   "_" + std::to_string(eng);

      VCL::DescriptorSet index(index_filename, unsigned(d), eng);

      int offset = 10;
      std::vector<long> classes = classes_increasing_offset(nb, offset);

      index.set_labels_map(class_map);

      index.add(xb, nb, classes);

      index.train();

      std::vector<float> distances;
      std::vector<long> desc_ids;
      index.search(xb, 1, 4, desc_ids, distances);

      int exp = 0;
      for (auto &desc : desc_ids) {
        EXPECT_EQ(desc, exp++);
      }

      std::vector<long> ret_ids = index.classify(xb, 60);
      std::vector<std::string> ret = index.label_id_to_string(ret_ids);

      for (int i = 0; i < offset; ++i) {
        EXPECT_EQ(ret[i], "parrot");
        EXPECT_EQ(ret[i + offset], "dog");
        EXPECT_EQ(ret[i + 2 * offset], "cat");
        EXPECT_EQ(ret[i + 3 * offset], "messi");
        EXPECT_EQ(ret[i + 4 * offset], "bird");
        EXPECT_EQ(ret[i + 5 * offset], "condor");
      }

      index.search(xb, 1, offset, desc_ids, distances);
      ret = index.get_str_labels(desc_ids);

      for (auto &label : ret) {
        EXPECT_EQ(label, "parrot");
      }

      index.store();
    }

    delete[] xb;
  }
}

TEST(Descriptors_Train, train_flatl2_4d_str_label) {
  int d = 4;
  int nb = 10000;

  float *xb = generate_desc_linear_increase(d, nb);

  std::string index_filename = "dbs/train_flatl2_4d_str_label.faiss";
  VCL::DescriptorSet index(index_filename, unsigned(d), VCL::FaissFlat);

  auto class_map = animals_map();
  index.set_labels_map(class_map);

  int offset = 10;
  std::vector<long> classes = classes_increasing_offset(nb, offset);

  index.add(xb, nb, classes);
  index.train();

  std::vector<float> distances;
  std::vector<long> desc_ids;
  index.search(xb, 1, 4, desc_ids, distances);

  int exp = 0;
  for (auto &desc : desc_ids) {
    EXPECT_EQ(desc, exp++);
  }

  int results[] = {0, 4, 16, 36};
  for (int i = 0; i < 4; ++i) {
    EXPECT_EQ(distances[i], results[i]);
  }

  std::vector<long> ret_ids = index.classify(xb, 60);

  std::vector<std::string> ret = index.label_id_to_string(ret_ids);

  for (int i = 0; i < offset; ++i) {
    EXPECT_EQ(ret[i], "parrot");
    EXPECT_EQ(ret[i + offset], "dog");
    EXPECT_EQ(ret[i + 2 * offset], "cat");
    EXPECT_EQ(ret[i + 3 * offset], "messi");
    EXPECT_EQ(ret[i + 4 * offset], "bird");
    EXPECT_EQ(ret[i + 5 * offset], "condor");
  }

  desc_ids.clear();
  distances.clear();

  index.search(xb, 1, offset, desc_ids, distances);
  ret = index.get_str_labels(desc_ids);

  for (auto &label : ret) {
    EXPECT_EQ(label, "parrot");
  }

  index.store();

  delete[] xb;
}

// TILEDBDense tests

TEST(Descriptors_Train, train_tdbdense_4d) {
  int d = 4;
  int nb = 10000;

  float *xb = generate_desc_linear_increase(d, nb);

  auto class_map = animals_map();

  std::string index_filename = "dbs/train_tdbdense_4d";
  VCL::DescriptorSet index(index_filename, unsigned(d), VCL::TileDBDense);

  index.set_labels_map(class_map);

  int offset = 10;
  std::vector<long> classes = classes_increasing_offset(nb, offset);

  index.add(xb, nb, classes);
  index.train();

  std::vector<float> distances;
  std::vector<long> desc_ids;
  index.search(xb, 1, 4, desc_ids, distances);

  int exp = 0;
  for (auto &desc : desc_ids) {
    EXPECT_EQ(desc, exp++);
  }

  int results[] = {0, 4, 16, 36};
  for (int i = 0; i < 4; ++i) {
    EXPECT_EQ(distances[i], results[i]);
  }

  std::vector<long> ret_ids = index.classify(xb, 60);

  std::vector<std::string> ret = index.label_id_to_string(ret_ids);

  for (int i = 0; i < offset; ++i) {
    EXPECT_EQ(ret[i], "parrot");
    EXPECT_EQ(ret[i + offset], "dog");
    EXPECT_EQ(ret[i + 2 * offset], "cat");
    EXPECT_EQ(ret[i + 3 * offset], "messi");
    EXPECT_EQ(ret[i + 4 * offset], "bird");
    EXPECT_EQ(ret[i + 5 * offset], "condor");
  }

  desc_ids.clear();
  distances.clear();

  index.search(xb, 1, offset, desc_ids, distances);
  ret = index.get_str_labels(desc_ids);

  for (auto &label : ret) {
    // std::cout << label << std::endl;
    EXPECT_EQ(label, "parrot");
  }

  index.store();

  delete[] xb;
}
