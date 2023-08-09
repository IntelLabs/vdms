/**
 * @file   helpers.cc
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2017 Intel Corporation
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

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string.h> // memcmp

#include "gtest/gtest.h"

#include "Exception.h"
#include "helpers.h"

#include <random>
#include <vector>

// Image / Video Helpers

void compare_mat_mat(cv::Mat &cv_img, cv::Mat &img, float error) {
  bool exact_comparison = (error == 0.0);

  ASSERT_EQ(cv_img.rows, img.rows);
  ASSERT_EQ(cv_img.cols, img.cols);
  ASSERT_EQ(cv_img.channels(), img.channels());

  int rows = img.rows;
  int columns = img.cols;
  int channels = img.channels();
  if (channels > 3) {
    throw VCLException(OpenFailed, "Greater than 3 channels in image");
  }

  // We make then continuous for faster comparison, if exact.
  if (exact_comparison && !img.isContinuous()) {
    cv::Mat aux = cv_img.clone();
    cv_img = aux.clone();
    aux = img.clone();
    img = aux.clone();
  }

  // For exact comparison, we use memcmp.
  if (exact_comparison) {
    size_t data_size = rows * columns * channels;
    int ret = ::memcmp(cv_img.data, img.data, data_size);
    ASSERT_EQ(ret, 0);
    return;
  }

  // For debugging, or near comparison, we check value by value.

  for (int i = 0; i < rows; ++i) {
    for (int j = 0; j < columns; ++j) {
      if (channels == 1) {
        unsigned char pixel = img.at<unsigned char>(i, j);
        unsigned char test_pixel = cv_img.at<unsigned char>(i, j);
        if (exact_comparison)
          ASSERT_EQ(pixel, test_pixel);
        else
          ASSERT_NEAR(pixel, test_pixel, error);
      } else {
        cv::Vec3b colors = img.at<cv::Vec3b>(i, j);
        cv::Vec3b test_colors = cv_img.at<cv::Vec3b>(i, j);
        for (int x = 0; x < channels; ++x) {
          if (exact_comparison)
            ASSERT_EQ(colors.val[x], test_colors.val[x]);
          else
            ASSERT_NEAR(colors.val[x], test_colors.val[x], error);
        }
      }
    }
  }
}

void compare_cvcapture_cvcapture(cv::VideoCapture v1, cv::VideoCapture v2) {
  while (true) {
    cv::Mat frame1;
    cv::Mat frame2;
    if (v1.read(frame1) && v2.read(frame2)) {
      if (!frame1.empty() && !frame2.empty()) {
        compare_mat_mat(frame1, frame2);
      } else if (frame1.empty() && frame2.empty()) {
        return;
      } else
        throw VCLException(ObjectEmpty, "One video ended before");
    } else
      throw VCLException(ObjectEmpty, "Error reading frames");
  }
}

// Descriptors Helpers

// This function return nb descriptors of dimension d as follows:
// init       init      ...   init      (d times)
// init+1     init+1    ...   init+1    (d times)
// ...
// init+nb-1  init+nb-1 ...   init+nb-1 (d times)

void generate_desc_linear_increase(int d, int nb, float *xb, float init) {
  float val = init;
  for (int i = 1; i <= nb * d; ++i) {
    xb[i - 1] = val;
    if (i % d == 0)
      val++;
  }
}

float *generate_desc_linear_increase(int d, int nb, float init) {
  float *xb = new float[d * nb];
  generate_desc_linear_increase(d, nb, xb, init);
  return xb;
}

void generate_desc_normal_cluster(int d, int nb, float *xb, float init,
                                  int cluster_size, float clusterhead_std,
                                  float cluster_std) {
  // std::cout << "\n Creating a Clustered Dataset ... \n";
  // std::cout << "nb= " <<nb <<" cluster size = " <<cluster_size << " d= "
  // <<d<<"... \n";

  std::srand(init);
  std::default_random_engine gen;
  std::normal_distribution<float> cluster_head_dist(
      0.0f,
      clusterhead_std); // cluster head standard deviation can be arbitrary
  std::normal_distribution<float> cluster_dist(
      0.0f, cluster_std); // cluster (neighbors close to cluster head) standard
                          // deviation should be a small noise (e.g. 1%-10%)

  if ((nb % cluster_size) != 0) {
    std::cout << "NOTE: Clustered Dataset Not Balanced, total number of "
                 "elements not a multiple of cluster size, clusters will not "
                 "have same number of items\n";
  }

  int n_clusters = floor((nb / cluster_size));
  int total = (floor((nb / cluster_size)) * cluster_size);
  int remaining = nb - total;

  std::vector<float> cluster_head(n_clusters * d);

  for (uint64_t i = 0; i < cluster_head.size();
       ++i) { // create cluster heads, they will be used as the list queries
    cluster_head[i] = cluster_head_dist(gen);
  }

  // create total dataset, cluster heads with neighbors around each
  for (int i = 0; i < n_clusters; i++) {
    for (int j = 0; j < cluster_size; j++) {
      if ((i * cluster_size + j) % cluster_size == 0) { // cluster head
        for (int z = 0; z < d; z++)
          xb[d * (i * cluster_size + j) + z] = cluster_head[i * d + z];
      } else { // cluster neighbor
        for (int z = 0; z < d; z++)
          xb[d * (i * cluster_size + j) + z] =
              cluster_head[i * d + z] + cluster_dist(gen);
      }
    }
  }
  for (int i = 0; i < remaining; i++) {
    for (int z = 0; z < d; z++) {
      xb[total * d + i * d + z] =
          cluster_head[n_clusters * d + z] + cluster_dist(gen);
    }
  }
  // end create total dataset
}

float *generate_desc_normal_cluster(int d, int nb, float init, int cluster_size,
                                    float clusterhead_std, float cluster_std) {
  float *xb = new float[d * nb]; // total dataset
  generate_desc_normal_cluster(d, nb, xb, init, cluster_size, clusterhead_std,
                               cluster_std);
  return xb;
}

void create_additional_neighbors(int d, int cluster_increment, int n_clusters,
                                 float *cluster_heads, float cluster_std,
                                 float *neighbors) {

  std::default_random_engine gen;
  std::normal_distribution<float> cluster_dist(
      0.0f, cluster_std); // cluster (neighbors close to cluster head) standard
                          // deviation should be a small noise (e.g. 1%-10%)

  // create increment neighbors dataset as new neihgbors near cluster heads
  for (int i = 0; i < n_clusters; i++) {
    for (int j = 0; j < cluster_increment; j++) {
      for (int z = 0; z < d; z++) {
        neighbors[d * (i * cluster_increment + j) + z] =
            cluster_heads[i * d + z] + cluster_dist(gen);
      }
    }
  }
}

float *create_additional_neighbors(int d, int cluster_increment, int n_clusters,
                                   float *cluster_heads, float cluster_std) {
  float *neighbors = new float[d * cluster_increment *
                               n_clusters]; // total additional neighbors
  create_additional_neighbors(d, cluster_increment, n_clusters, cluster_heads,
                              cluster_std, neighbors);
  return neighbors;
}

std::map<long, std::string> animals_map() {
  std::map<long, std::string> class_map;
  class_map[0] = "parrot";
  class_map[1] = "dog";
  class_map[2] = "cat";
  class_map[3] = "messi";
  class_map[4] = "bird";
  class_map[5] = "condor";
  class_map[6] = "panda";

  return class_map;
}

std::vector<long> classes_increasing_offset(unsigned nb, unsigned offset) {
  std::vector<long> classes(nb, 0);

  for (int i = 0; i < nb / offset; ++i) {
    for (int j = 0; j < offset; ++j) {
      classes[i * offset + j] = i;
    }
  }

  return classes;
}

std::vector<VCL::DescriptorSetEngine> get_engines() {
  std::vector<VCL::DescriptorSetEngine> engs;
  engs.push_back(VCL::FaissFlat);
  engs.push_back(VCL::FaissIVFFlat);
  engs.push_back(VCL::TileDBDense);
  engs.push_back(VCL::TileDBSparse);
  // engs.push_back(VCL::Flinng);
  // FLINNG only supports normalized dataset
  // disable general tests until support for arbitrary datasets is added

  return engs;
}

std::list<int> get_dimensions_list() {
  // std::list<int> dims = {64, 97, 128, 256, 300, 453, 1000, 1024, 2045};
  // std::list<int> dims = {128, 300, 453, 1024};
  // std::list<int> dims = {128, 300, 453};
  // std::list<int> dims = {128, 255};
  std::list<int> dims = {128};

  return dims;
}
