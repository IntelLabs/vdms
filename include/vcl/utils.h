/**
 * @file   utils.h
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

#pragma once

#include <time.h>
#include <vector>

namespace VCL {

typedef std::vector<unsigned char> cv_buffer;

/*  *********************** */
/*          ENUMS           */
/*  *********************** */
/**
 *  Determines what kind of compression to use
 */

enum class CompressionType {
  NOCOMPRESSION = 0,
  GZIP = 1,
  ZSTD = 2,
  LZ4 = 3,
  BLOSC = 4,
  BLZ4 = 5,
  BLZ4HC = 6,
  BSNAPPY = 7,
  BZLIB = 8,
  BZSTD = 9,
  RLE = 10
};

enum class Storage { LOCAL = 0, AWS = 1 };

static const struct init_rand_t {
  init_rand_t() { srand(time(NULL)); }
} init_rand;

uint64_t rdrand();

bool supports_rdrand();

uint64_t get_uint64();

/**
 *  Gets the extension of a filename
 *
 *  @param filename  The path to the file
 *  @return The string containing the extension
 */
std::string get_extension(const std::string &filename);

/**
 *  Checks to see if the file name is unique by attempting
 *    to open the file
 *
 *  @param name  Full path to the theoretically unique ID
 *  @return True if the file does not exist, false if it does
 */
bool exists(const std::string &name);

std::string create_unique(const std::string &path,
                          const std::string &extension);
}; // namespace VCL
