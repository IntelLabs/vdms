/**
 * @file   DescriptorSetClassify_test.cc
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

TEST(Descriptors_Classify, classify_flatl2_4d) {
  int d = 4;
  int nb = 10000;

  float *xb = generate_desc_linear_increase(d, nb);

  std::string index_filename = "dbs/classify_flatl2_4d.faiss";
  VCL::DescriptorSet index(index_filename, unsigned(d), VCL::FaissFlat);

  int offset = 10;
  std::vector<long> classes = classes_increasing_offset(nb, offset);

  index.add(xb, nb, classes);

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

TEST(Descriptors_Classify, classify_10k) {
  int nb = 10000;

  auto dimensions_list = get_dimensions_list();

  for (auto d : dimensions_list) {

    float *xb = generate_desc_linear_increase(d, nb);

    for (auto eng : get_engines()) {
      std::string index_filename =
          "dbs/classify_10k" + std::to_string(d) + "_" + std::to_string(eng);

      VCL::DescriptorSet index(index_filename, unsigned(d), eng);

      int offset = 10;
      std::vector<long> classes = classes_increasing_offset(nb, offset);

      index.add(xb, nb, classes);

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

TEST(Descriptors_Classify, classify_ivfflatl2_4d_labels) {
  int d = 4;
  int nb = 10000;

  float *xb = generate_desc_linear_increase(d, nb);

  auto class_map = animals_map();

  std::string index_filename = "dbs/classify_ivfflatl2_4d_labels.faiss";
  VCL::DescriptorSet index(index_filename, unsigned(d), VCL::FaissIVFFlat);

  int offset = 10;
  std::vector<long> classes = classes_increasing_offset(nb, offset);

  index.set_labels_map(class_map);

  index.add(xb, nb, classes);

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

TEST(Descriptors_Classify, classify_flinngIP_100d_labels) {
  int d = 100;
  int nb = 10000;

  float init = 0.0;
  int offset = 10;
  float clusterhead_std = 1.0;
  float cluster_std = 0.1;

  int n_clusters = floor((nb / offset));

  float *xb = generate_desc_normal_cluster(d, nb, init, offset, clusterhead_std,
                                           cluster_std);
  std::string index_filename = "dbs/classify_flinngIP_100d_labels";

  VCL::DescriptorParams *param = new VCL::DescriptorParams(3, nb / 10, 10, 12);
  VCL::DescriptorSet index(index_filename, unsigned(d), VCL::Flinng,
                           VCL::DistanceMetric::IP, param);

  /*
  std::vector<long> classes(nb);

  for (int i = 0; i < n_clusters ; i++) {
      for (int j = 0; j < offset; j++){
          classes[i*offset + j] =  i;
      }
  }
  */

  auto class_map = animals_map();
  std::vector<long> classes = classes_increasing_offset(nb, offset);
  index.set_labels_map(class_map);

  index.add_and_store(xb, nb, classes);
  index.finalize_index();

  std::vector<float> cluster_head(n_clusters * d);
  std::vector<long> descriptors(n_clusters * offset);

  for (int i = 0; i < n_clusters; i++) {
    for (int j = 0; j < offset; j++) {
      if ((i * offset + j) % offset == 0) {
        for (int z = 0; z < d; z++)
          cluster_head[i * d + z] = xb[d * (i * offset + j) + z];
      }
    }
  }

  index.search(cluster_head.data(), n_clusters, offset, descriptors);

  int correct = 0;
  float recall = 0.0;
  for (int i = 0; i < n_clusters; ++i) {
    for (int j = 0; j < offset; ++j) {
      if ((i * offset <= descriptors[i * offset + j]) &&
          (descriptors[i * offset + j] < (i + 1) * offset))
        correct++;
    }
  }
  recall = static_cast<float>(correct) / (n_clusters * offset);
  EXPECT_GE(recall, 0.7);

  std::vector<long> desc_ids;
  index.search(xb, 1, offset, desc_ids);

  correct = 0;
  recall = 0.0;
  for (int j = 0; j < offset; ++j) {
    if ((0 <= desc_ids[j]) && (desc_ids[j] < offset)) {
      correct++;
    }
  }

  recall = static_cast<float>(correct) / offset;
  EXPECT_GE(recall, 0.7);

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

  index.search(xb, 1, offset, desc_ids);
  ret = index.get_str_labels(desc_ids);

  for (auto &label : ret) {
    EXPECT_EQ(label, "parrot");
  }

  delete[] xb;
}

TEST(Descriptors_Classify, classify_labels_10k) {
  int nb = 10000;

  auto dimensions_list = get_dimensions_list();
  auto class_map = animals_map();

  for (auto d : dimensions_list) {
    float *xb = generate_desc_linear_increase(d, nb);

    for (auto eng : get_engines()) {
      std::string index_filename = "dbs/classify_labels_10k_" +
                                   std::to_string(d) + "_" +
                                   std::to_string(eng);

      VCL::DescriptorSet index(index_filename, unsigned(d), eng);

      int offset = 10;
      std::vector<long> classes = classes_increasing_offset(nb, offset);

      index.set_labels_map(class_map);

      index.add(xb, nb, classes);

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

TEST(Descriptors_Classify, classify_flatl2_4d_str_label) {
  int d = 4;
  int nb = 10000;

  float *xb = generate_desc_linear_increase(d, nb);

  std::string index_filename = "dbs/classify_flatl2_4d_str_label.faiss";
  VCL::DescriptorSet index(index_filename, unsigned(d), VCL::FaissFlat);

  auto class_map = animals_map();
  index.set_labels_map(class_map);

  int offset = 10;
  std::vector<long> classes = classes_increasing_offset(nb, offset);

  index.add(xb, nb, classes);

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

TEST(Descriptors_Classify, classify_tdbdense_4d) {
  int d = 4;
  int nb = 10000;

  float *xb = generate_desc_linear_increase(d, nb);

  auto class_map = animals_map();

  std::string index_filename = "dbs/classify_tdbdense_4d";
  VCL::DescriptorSet index(index_filename, unsigned(d), VCL::TileDBDense);

  index.set_labels_map(class_map);

  int offset = 10;
  std::vector<long> classes = classes_increasing_offset(nb, offset);

  index.add(xb, nb, classes);

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
