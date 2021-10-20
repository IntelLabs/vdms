/**
 * @file   DescriptorSetAdd_test.cc
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

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <fstream>
#include <iostream>
#include <cmath>
#include <list>

#include "vcl/VCL.h"
#include "helpers.h"
#include "gtest/gtest.h"

TEST(Descriptors_Add, add_flatl2_100d)
{
    int d = 100;
    int nb = 10000;

    float *xb = generate_desc_linear_increase(d, nb);

    std::string index_filename = "dbs/add_flatl2_100d";
    VCL::DescriptorSet index(index_filename, unsigned(d), VCL::FaissFlat);

    index.add(xb, nb);

    std::vector<float> distances;
    std::vector<long> desc_ids;
    index.search(xb, 1, 4, desc_ids, distances);

    int exp = 0;
    // std::cout << "DescriptorSet: " << std::endl;
    for (auto& desc : desc_ids) {
        // std::cout << desc << " ";
        EXPECT_EQ(desc, exp++);
    }

    // std::cout << "Distances: " << std::endl;
    float results[] = {float(std::pow(0, 2)*d),
                       float(std::pow(1, 2)*d),
                       float(std::pow(2, 2)*d),
                       float(std::pow(3, 2)*d) };

    for (int i = 0; i < 4; ++i) {
        // std::cout << distances[i] <<  " ";
        EXPECT_EQ(distances[i], results[i]);
    }
    // std::cout << std::endl;

    index.store();

    delete [] xb;
}

TEST(Descriptors_Add, add_and_radius_search_flatl2_100d)
{
    int d = 100;
    int nb = 10000;

    float *xb = generate_desc_linear_increase(d, nb);

    std::string index_filename = "dbs/add_and_radius_search_flatl2_100d";
    VCL::DescriptorSet index(index_filename, unsigned(d), VCL::FaissFlat);

    index.add(xb, nb);

    long* desc_ids   = new long [20];
    float* distances = new float[20];
    index.radius_search(xb, 2000, desc_ids, distances);

    int exp = 0;

    float results[] = {float(std::pow(0, 2)*d),
                       float(std::pow(1, 2)*d),
                       float(std::pow(2, 2)*d),
                       float(std::pow(3, 2)*d) };

    for (int i = 0; i < 4; ++i) {
        // std::cout << distances[i] <<  " ";
        EXPECT_EQ(distances[i], results[i]);
    }
    // std::cout << std::endl;

    index.store();

    delete [] xb;
}


TEST(Descriptors_Add, add_ivfflatl2_100d)
{
    int d = 100;
    int nb = 10000;
    float *xb = generate_desc_linear_increase(d, nb);

    std::string index_filename = "dbs/add_ivfflatl2_100d";
    VCL::DescriptorSet index(index_filename, unsigned(d), VCL::FaissIVFFlat);

    std::vector<long> classes(nb);

    for (auto& str : classes) {
        str = 1;
    }

    index.add(xb, nb, classes);

    std::vector<float> distances;
    std::vector<long> desc_ids;
    index.search(xb, 1, 4, desc_ids, distances);

    int exp = 0;
    // std::cout << "DescriptorSet: " << std::endl;
    for (auto& desc : desc_ids) {
        // std::cout << desc << " ";
        EXPECT_EQ(desc, exp++);
    }
    // std::cout << std::endl;

    // std::cout << "Distances: " << std::endl;
    float results[] = {float(std::pow(0, 2)*d),
                       float(std::pow(1, 2)*d),
                       float(std::pow(2, 2)*d),
                       float(std::pow(3, 2)*d) };
    for (int i = 0; i < 4; ++i) {
        // std::cout << distances[i] <<  " ";
        EXPECT_EQ(distances[i], results[i]);
    }
    // std::cout << std::endl;

    index.store();

    delete [] xb;
}

TEST(Descriptors_Add, add_recons_flatl2_100d)
{
    int d = 100;
    int nb = 10000;
    float *xb = generate_desc_linear_increase(d, nb);

    std::string index_filename = "dbs/add_recons_flatl2_100d";
    VCL::DescriptorSet index(index_filename, unsigned(d), VCL::FaissFlat);

    std::vector<long> classes(nb);

    for (auto& cl : classes) {
        cl = 1;
    }

    index.add(xb, nb, classes);

    std::vector<float> distances;
    std::vector<long> desc_ids;
    index.search(xb, 1, 4, desc_ids, distances);
    desc_ids.clear();

    float *recons = new float[d * nb];
    for (int i = 0; i < nb; ++i) {
        desc_ids.push_back(i);
    }

    index.get_descriptors(desc_ids, recons);

    for (int i = 0; i < nb*d; ++i) {
        EXPECT_EQ(xb[i], recons[i]);
    }

    index.store();

    delete [] xb;
}

TEST(Descriptors_Add, add_flatl2_100d_2add)
{
    int d = 100;
    int nb = 10000;
    float *xb = generate_desc_linear_increase(d, nb);

    std::string index_filename = "dbs/add_flatl2_100d_2add";
    VCL::DescriptorSet index(index_filename, unsigned(d), VCL::FaissFlat);

    index.add(xb, nb);

    generate_desc_linear_increase(d, nb, xb, .6);

    index.add(xb, nb);

    generate_desc_linear_increase(d, 4, xb, 0);

    std::vector<float> distances;
    std::vector<long> desc_ids;
    index.search(xb, 1, 4, desc_ids, distances);

    float results[] = {float(std::pow(0, 2)*d),
                       float(std::pow(.6, 2)*d),
                       float(std::pow(1, 2)*d),
                       float(std::pow(1.6, 2)*d) };
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(std::round(distances[i]), std::round(results[i]));
    }

    index.store();
    delete [] xb;
}

// TileDB Dense Tests

TEST(Descriptors_Add, add_tiledbdense_100d)
{
    int d = 100;
    int nb = 10000;
    float *xb = generate_desc_linear_increase(d, nb);

    std::string index_filename = "dbs/add_tiledbdense_100d_tdb";
    VCL::DescriptorSet index(index_filename, unsigned(d), VCL::TileDBDense);

    index.add(xb, nb);

    std::vector<float> distances;
    std::vector<long> desc_ids;
    index.search(xb, 1, 4, desc_ids, distances);

    int exp = 0;
    // std::cout << "DescriptorSet: " << std::endl;
    for (auto& desc : desc_ids) {
        // std::cout << desc << " ";
        EXPECT_EQ(desc, exp++);
    }

    // std::cout << "Distances: " << std::endl;
    float results[] = {float(std::pow(0, 2)*d),
                       float(std::pow(1, 2)*d),
                       float(std::pow(2, 2)*d),
                       float(std::pow(3, 2)*d) };
    for (int i = 0; i < 4; ++i) {
        // std::cout << distances[i] <<  " ";
        EXPECT_EQ(distances[i], results[i]);
    }
    // std::cout << std::endl;

    index.store();

    delete [] xb;
}

TEST(Descriptors_Add, add_tiledbdense_100d_2add)
{
    int d = 100;
    int nb = 10000;
    float *xb = generate_desc_linear_increase(d, nb);

    std::string index_filename = "dbs/add_tiledbdense_100d_2add";
    VCL::DescriptorSet index(index_filename, unsigned(d), VCL::TileDBDense);

    index.add(xb, nb);

    generate_desc_linear_increase(d, nb, xb, .6);

    index.add(xb, nb);

    generate_desc_linear_increase(d, 4, xb, 0);

    std::vector<float> distances;
    std::vector<long> desc_ids;
    index.search(xb, 1, 4, desc_ids, distances);

    float results[] = {float(std::pow(0, 2)*d),
                       float(std::pow(.6, 2)*d),
                       float(std::pow(1, 2)*d),
                       float(std::pow(1.6, 2)*d) };
    // This is:
    //  (0)  ^2 * 100 = 0
    //  (0.6)^2 * 100 = 36
    //  (1  )^2 * 100 = 100
    //  (1.6)^2 * 100 = 256

    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(std::round(distances[i]), std::round(results[i]));
        // printf(" %f, %f \n", float(distances[i]), float(results[i]));
    }

    index.store();
    delete [] xb;
}

// TileDB Sparse

#define TDB_SPARSE
#ifdef TDB_SPARSE

TEST(Descriptors_Add, add_tiledbsparse_100d_2add)
{
    int d = 100;
    int nb = 10000;
    float *xb = generate_desc_linear_increase(d, nb);
    // generate_desc_linear_increase(d, nb, xb, .1);

    std::string index_filename = "dbs/add_tiledbsparse_100d_2add";
    VCL::DescriptorSet index(index_filename, unsigned(d), VCL::TileDBSparse);

    index.add(xb, nb);

    generate_desc_linear_increase(d, nb, xb, .6);

    index.add(xb, nb);

    generate_desc_linear_increase(d, 4, xb, 0);

    std::vector<float> distances;
    std::vector<long> desc_ids;
    index.search(xb, 2, 4, desc_ids, distances);

    float results[] = {float(std::pow(0, 2)*d),
                       float(std::pow(.6, 2)*d),
                       float(std::pow(1, 2)*d),
                       float(std::pow(1.6, 2)*d) };

    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(std::round(distances[i]), std::round(results[i]));
    }

    index.store();
    delete [] xb;
}

TEST(Descriptors_Add, add_tiledbsparse_100d)
{
    int d = 100;
    int nb = 10000;
    float *xb = generate_desc_linear_increase(d, nb);
    // generate_desc_linear_increase(d, nb, xb, .1);

    std::string index_filename = "dbs/add_tiledbsparse_100d";
    VCL::DescriptorSet index(index_filename, unsigned(d), VCL::TileDBSparse);

    index.add(xb, nb);

    std::vector<float> distances;
    std::vector<long> desc_ids;
    index.search(xb, 1, 4, desc_ids, distances);

    float results[] = {float(std::pow(0, 2)*d),
                       float(std::pow(1, 2)*d),
                       float(std::pow(2, 2)*d),
                       float(std::pow(3, 2)*d) };

    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(std::round(distances[i]), std::round(results[i]));
    }

    index.store();
    delete [] xb;
}

TEST(Descriptors_Add, add_2_times_same_tdbsparse)
{
    int nb = 10000;

    auto dimensions_list = get_dimensions_list();

    for (auto d : dimensions_list) {

        float *xb = generate_desc_linear_increase(d, nb);

        auto eng = VCL::TileDBSparse;

        std::string index_filename = "dbs/add_2_times_same_tdbsparse_" +
                                     std::to_string(d) + "_" +
                                     std::to_string(eng);

        VCL::DescriptorSet index(index_filename, unsigned(d), eng);

        index.add(xb, nb);

        generate_desc_linear_increase(d, nb, xb, 10000);

        index.add(xb, nb);

        generate_desc_linear_increase(d, 4, xb, 0);

        std::vector<float> distances;
        std::vector<long> desc_ids;
        index.search(xb, 1, 4, desc_ids, distances);

        float results[] = {float(std::pow(0, 2)*d),
                           float(std::pow(1, 2)*d),
                           float(std::pow(2, 2)*d),
                           float(std::pow(3, 2)*d) };

        for (int i = 0; i < 4; ++i) {
            EXPECT_NEAR((distances[i]), (results[i]), .5f);
        }

        delete [] xb;
    }
}

TEST(Descriptors_Add, add_2_times_tdbsparse)
{
    int nb = 10000;

    auto dimensions_list = get_dimensions_list();

    for (auto d : dimensions_list) {

        float *xb = generate_desc_linear_increase(d, nb);

        auto eng = VCL::TileDBSparse;

        std::string index_filename = "dbs/add_2_times_tdbsparse_" +
                                     std::to_string(d) + "_" +
                                     std::to_string(eng);

        VCL::DescriptorSet index(index_filename, unsigned(d), eng);

        index.add(xb, nb);

        generate_desc_linear_increase(d, nb, xb, .6);

        index.add(xb, nb);

        generate_desc_linear_increase(d, 4, xb, 0);

        std::vector<float> distances;
        std::vector<long> desc_ids;
        index.search(xb, 1, 4, desc_ids, distances);

        float results[] = {float(std::pow(0, 2)*d),
                           float(std::pow(.6, 2)*d),
                           float(std::pow(1, 2)*d),
                           float(std::pow(1.6, 2)*d) };

        for (int i = 0; i < 4; ++i) {
            EXPECT_NEAR((distances[i]), (results[i]), .5f);
        }

        delete [] xb;
    }
}

#endif

// ----------

TEST(Descriptors_Add, add_and_search_10k)
{
    int nb = 10000;
    auto dimensions_list = get_dimensions_list();

    for (auto d : dimensions_list) {

        float *xb = generate_desc_linear_increase(d, nb);

        for (auto eng : get_engines()) {
            std::string index_filename = "dbs/add_and_search_10k" +
                                         std::to_string(d) + "_" +
                                         std::to_string(eng);

            VCL::DescriptorSet index(index_filename, unsigned(d), eng);

            index.add(xb, nb);

            std::vector<float> distances;
            std::vector<long> desc_ids;
            index.search(xb, 1, 4, desc_ids, distances);

            int exp = 0;
            // std::cout << "DescriptorSet: " << std::endl;
            for (auto& desc : desc_ids) {
                // std::cout << desc << " ";
                EXPECT_EQ(desc, exp++);
            }

            // std::cout << "Distances: " << std::endl;
            float results[] = {float(std::pow(0, 2)*d),
                               float(std::pow(1, 2)*d),
                               float(std::pow(2, 2)*d),
                               float(std::pow(3, 2)*d) };
            for (int i = 0; i < 4; ++i) {
                // std::cout << distances[i] <<  " ";
                EXPECT_EQ(distances[i], results[i]);
            }
            // std::cout << std::endl;

            index.store();
        }

        delete [] xb;
    }
}

TEST(Descriptors_Add, add_and_search_10k_negative)
{
    int nb = 10000;
    auto dimensions_list = get_dimensions_list();

    for (auto d : dimensions_list) {

        float *xb = generate_desc_linear_increase(d, nb, -900);

        for (auto eng : get_engines()) {
            std::string index_filename = "dbs/add_and_search_10k_negative" +
                                         std::to_string(d) + "_" +
                                         std::to_string(eng);

            VCL::DescriptorSet index(index_filename, unsigned(d), eng);

            index.add(xb, nb);

            std::vector<float> distances;
            std::vector<long> desc_ids;
            index.search(xb, 1, 4, desc_ids, distances);

            int exp = 0;
            for (auto& desc : desc_ids) {
                EXPECT_EQ(desc, exp++);
            }

            float results[] = {float(std::pow(0, 2)*d),
                               float(std::pow(1, 2)*d),
                               float(std::pow(2, 2)*d),
                               float(std::pow(3, 2)*d) };
            for (int i = 0; i < 4; ++i) {
                EXPECT_EQ(distances[i], results[i]);
            }

            index.store();
        }

        delete [] xb;
    }
}

TEST(Descriptors_Add, add_1by1_and_search_1k)
{
    int nb = 1000;
    auto dimensions_list = get_dimensions_list();

    for (auto d : dimensions_list) {

        float *xb = generate_desc_linear_increase(d, nb);

        for (auto eng : get_engines()) {

            // It does not make sense to run on this index
            if (eng == VCL::FaissIVFFlat)
                continue;

            std::string index_filename = "dbs/add_1by1_and_search_1k_" +
                                         std::to_string(d) + "_" +
                                         std::to_string(eng);

            VCL::DescriptorSet index(index_filename, unsigned(d), eng);

            printf("eng: %d \n",eng );
            for (int i = 0; i < nb; ++i) {
                index.add(xb + i*d, 1);
            }

            printf("about to start search... \n");
            std::vector<float> distances;
            std::vector<long> desc_ids;
            index.search(xb, 1, 4, desc_ids, distances);

            printf("done search\n");

            int exp = 0;
            for (auto& desc : desc_ids) {
                EXPECT_EQ(desc, exp++);
            }

            float results[] = {float(std::pow(0, 2)*d),
                               float(std::pow(1, 2)*d),
                               float(std::pow(2, 2)*d),
                               float(std::pow(3, 2)*d) };
            for (int i = 0; i < 4; ++i) {
                EXPECT_EQ(distances[i], results[i]);
            }

            index.store();
            printf("done store\n");
        }

        delete [] xb;
    }
}

TEST(Descriptors_Add, add_and_search_2_neigh_10k)
{
    int nb = 10000;
    auto dimensions_list = get_dimensions_list();

    for (auto d : dimensions_list) {

        float *xb = generate_desc_linear_increase(d, nb);

        for (auto eng : get_engines()) {
            std::string index_filename = "dbs/add_and_search_2_neigh_10k" +
                                         std::to_string(d) + "_" +
                                         std::to_string(eng);

            VCL::DescriptorSet index(index_filename, unsigned(d), eng);

            index.add(xb, nb);

            std::vector<float> distances;
            std::vector<long> desc_ids;
            index.search(xb, 2, 4, desc_ids, distances);

            // Does not matter much, but good to test
            // int exp[] = {0, 1, 2, 3, 1, 2, 0, 3};
            // int idx = 0;
            // // std::cout << "DescriptorSet: " << std::endl;
            // for (auto& desc : desc_ids) {
            //     // std::cout << desc << " ";
            //     EXPECT_EQ(desc, exp[idx++]);
            // }

            // std::cout << "Distances: " << std::endl;
            float results[] = {float(std::pow(0, 2)*d),
                               float(std::pow(1, 2)*d),
                               float(std::pow(2, 2)*d),
                               float(std::pow(3, 2)*d) };
            for (int i = 0; i < 4; ++i) {
                // std::cout << distances[i] <<  " ";
                EXPECT_EQ(distances[i], results[i]);
            }

            float results_2[] = {float(std::pow(0, 2)*d),
                               float(std::pow(1, 2)*d),
                               float(std::pow(1, 2)*d),
                               float(std::pow(2, 2)*d) };

            for (int i = 4; i < 8; ++i) {
                // std::cout << distances[i] <<  " ";
                EXPECT_EQ(distances[i], results_2[i-4]);
            }
            // std::cout << std::endl;

            index.store();
        }

        delete [] xb;
    }
}

TEST(Descriptors_Add, add_2_times)
{
    // int d = 100;
    int nb = 10000;

    auto dimensions_list = get_dimensions_list();

    for (auto d : dimensions_list) {

        float *xb = generate_desc_linear_increase(d, nb);

        for (auto eng : get_engines()) {

            // this eng is segfaulting, possible tdb bug
            if (eng == VCL::TileDBSparse)
                continue;

            std::string index_filename = "dbs/add_2_times_" +
                                         std::to_string(d) + "_" +
                                         std::to_string(eng);

            VCL::DescriptorSet index(index_filename, unsigned(d), eng);

            index.add(xb, nb);

            generate_desc_linear_increase(d, nb, xb, .6);

            index.add(xb, nb);

            generate_desc_linear_increase(d, 4, xb, 0);

            std::vector<float> distances;
            std::vector<long> desc_ids;
            index.search(xb, 1, 4, desc_ids, distances);

            float results[] = {float(std::pow(0, 2)*d),
                               float(std::pow(.6, 2)*d),
                               float(std::pow(1, 2)*d),
                               float(std::pow(1.6, 2)*d) };

            for (int i = 0; i < 4; ++i) {
                EXPECT_NEAR((distances[i]), (results[i]), .5f);
            }
        }

        delete [] xb;
    }
}

TEST(Descriptors_Add, add_and_get_descriptors)
{
    int nb = 10000;

    int recons_n = 10;

    auto dimensions_list = get_dimensions_list();

    for (auto d : dimensions_list) {

        float *xb = generate_desc_linear_increase(d, nb);

        std::vector<long> recons_ids;
        for (int i = 0; i < recons_n; ++i) {
            recons_ids.push_back(i);
        }

        for (auto eng : get_engines()) {

            std::string index_filename = "dbs/add_and_get_descriptors_10k" +
                                         std::to_string(d) + "_" +
                                         std::to_string(eng);

            VCL::DescriptorSet index(index_filename, unsigned(d), eng);

            index.add(xb, nb);

            float *recons = new float[d * recons_n];
            index.get_descriptors(recons_ids, recons);

            for (int i = 0; i < recons_n*d; ++i) {
                EXPECT_NEAR(xb[i], recons[i], .01f);
            }
            // printf("%d\n", eng);

            delete[] recons;

            index.store();
        }

        delete [] xb;
    }
}
