/**
 * @file   helpers.h
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

#pragma once

#include <cstdio>
#include <cstdlib>
#include <list>

#include <opencv2/highgui.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include "vcl/VCL.h"

// Image / Video Helpers

void compare_mat_mat(cv::Mat &cv_img, cv::Mat &img, float error = 0.0);

void compare_cvcapture_cvcapture(cv::VideoCapture v1, cv::VideoCapture v2);

// Descriptors Helpers

void generate_desc_linear_increase(int d, int nb, float *xb, float init = 0);

float *generate_desc_linear_increase(int d, int nb, float init = 0);

void generate_desc_normal_cluster(int d, int nb, float *xb, float init = 0,
                                  int cluster_size = 5,
                                  float clusterhead_std = 1.0,
                                  float cluster_std = 0.1);

float *generate_desc_normal_cluster(int d, int nb, float init = 0,
                                    int cluster_size = 5,
                                    float clusterhead_std = 1.0,
                                    float cluster_std = 0.1);

void create_additional_neighbors(int d, int cluster_increment, int n_clusters,
                                 float *cluster_heads, float cluster_std,
                                 float *neighbors);

float *create_additional_neighbors(int d, int cluster_increment, int n_clusters,
                                   float *cluster_heads, float cluster_std);

void check_arrays_float(float *a, float *b, int d);

std::map<long, std::string> animals_map();

std::vector<long> classes_increasing_offset(unsigned nb, unsigned offset);

std::vector<VCL::DescriptorSetEngine> get_engines();

std::list<int> get_dimensions_list();
