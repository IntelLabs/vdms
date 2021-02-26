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

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <fstream>
#include <iostream>
#include <cmath>
#include <string.h> // memcmp

#include "gtest/gtest.h"

#include "helpers.h"

// Image / Video Helpers

void compare_mat_mat(cv::Mat &cv_img, cv::Mat &img, float error)
{
    bool exact_comparison = (error == 0.0);

    ASSERT_EQ(cv_img.rows, img.rows);
    ASSERT_EQ(cv_img.cols, img.cols);
    ASSERT_EQ(cv_img.channels(), img.channels());

    int rows     = img.rows;
    int columns  = img.cols;
    int channels = img.channels();

    // We make then continuous for faster comparison, if exact.
    if (exact_comparison && !img.isContinuous()) {
        cv::Mat aux = cv_img.clone();
        cv_img  = aux.clone();
        aux     = img.clone();
        img     = aux.clone();
    }

    // For exact comparison, we use memcmp.
    if (exact_comparison) {
        size_t data_size = rows * columns * channels;
        int ret = ::memcmp(cv_img.data, img.data, data_size);
        ASSERT_EQ(ret, 0);
        return;
    }

    // For debugging, or near comparison, we check value by value.

    for ( int i = 0; i < rows; ++i ) {
        for ( int j = 0; j < columns; ++j ) {
            if (channels == 1) {
                unsigned char pixel = img.at<unsigned char>(i, j);
                unsigned char test_pixel = cv_img.at<unsigned char>(i, j);
                if (exact_comparison)
                    ASSERT_EQ(pixel, test_pixel);
                else
                    ASSERT_NEAR(pixel, test_pixel, error);
            }
            else {
                cv::Vec3b colors = img.at<cv::Vec3b>(i, j);
                cv::Vec3b test_colors = cv_img.at<cv::Vec3b>(i, j);
                for ( int x = 0; x < channels; ++x ) {
                    if (exact_comparison)
                        ASSERT_EQ(colors.val[x], test_colors.val[x]);
                    else
                        ASSERT_NEAR(colors.val[x], test_colors.val[x], error);
                }
            }
        }
    }
}

void compare_cvcapture_cvcapture(cv::VideoCapture v1, cv::VideoCapture v2)
{
    while (true) {
        cv::Mat frame1;
        cv::Mat frame2;
        if ( v1.read(frame1) && v2.read(frame2)) {
            if ( !frame1.empty() && !frame2.empty()) {
                 compare_mat_mat(frame1,frame2);
            }
            else if ( frame1.empty() && frame2.empty()) {
                return;
            }
            else
                throw VCLException(ObjectEmpty, "One video ended before");
        }
        else
            throw VCLException(ObjectEmpty, "Error reading frames");
    }
}

// Descriptors Helpers

// This function return nb descriptors of dimension d as follows:
// init       init      ...   init      (d times)
// init+1     init+1    ...   init+1    (d times)
// ...
// init+nb-1  init+nb-1 ...   init+nb-1 (d times)

void generate_desc_linear_increase(int d, int nb, float* xb, float init)
{
    float val = init;
    for (int i = 1; i <= nb*d; ++i) {
        xb[i-1] = val;
        if ( i%d == 0) val++;
    }
}

float* generate_desc_linear_increase(int d, int nb, float init)
{
    float *xb = new float[d * nb];
    generate_desc_linear_increase(d, nb, xb, init);
    return xb;
}

std::map<long, std::string> animals_map()
{
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

std::vector<long> classes_increasing_offset(unsigned nb, unsigned offset)
{
    std::vector<long> classes(nb, 0);

    for (int i = 0; i < nb/offset; ++i) {
        for (int j = 0; j < offset; ++j) {
            classes[i*offset + j] = i;
        }
    }

    return classes;
}

std::vector<VCL::DescriptorSetEngine> get_engines()
{
    std::vector<VCL::DescriptorSetEngine> engs;
    engs.push_back(VCL::FaissFlat);
    engs.push_back(VCL::FaissIVFFlat);
    engs.push_back(VCL::TileDBDense);
    engs.push_back(VCL::TileDBSparse);

    return engs;
}

std::list<int> get_dimensions_list()
{
    // std::list<int> dims = {64, 97, 128, 256, 300, 453, 1000, 1024, 2045};
    // std::list<int> dims = {128, 300, 453, 1024};
    // std::list<int> dims = {128, 300, 453};
    // std::list<int> dims = {128, 255};
    std::list<int> dims = {128};

    return dims;
}
