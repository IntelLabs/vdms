/**
 * @file   utils.cc
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

#include <cpuid.h>
#include <sstream>
#include <string>
#include <sys/stat.h>

#include "../VDMSConfig.h"
#include "vcl/Exception.h"
#include "vcl/utils.h"

namespace VCL {

uint64_t rdrand() {
  static const unsigned retry_limit = 10;
  unsigned retries = retry_limit;
  do {
    uint64_t val;
    bool r;
    __asm("rdrand %0; setc %1" : "=r"(val), "=r"(r));
    if (r)
      return val;
  } while (--retries);

  throw VCLException(UndefinedException, "Random number not generated\n");
}

bool supports_rdrand() {
  const unsigned int flag_rdrand = (1 << 30);

  unsigned int eax, ebx, ecx, edx;
  __cpuid(1, eax, ebx, ecx, edx);

  return ((ecx & flag_rdrand) == flag_rdrand);
}

uint64_t combine(uint64_t a, uint64_t b) {
  int multiplier = 1;

  while (multiplier <= a) {
    multiplier *= 10;
  }

  return a * multiplier + b;
}

uint64_t get_uint64() {
  if (supports_rdrand()) {
    return combine(rdrand(), rdrand());
  } else {
    init_rand;

    return combine(rand(), rand());
  }
}

std::string get_extension(const std::string &object_id) {
  size_t file_ext = object_id.find_last_of(".");
  size_t dir_ext = object_id.find_last_of("/");

  if (file_ext != std::string::npos) {
    if (file_ext > dir_ext + 2)
      return object_id.substr(file_ext + 1);
    else
      throw VCLException(ObjectEmpty,
                         object_id + " does not have a valid extension");
  } else
    return "";
}

bool exists(const std::string &name) {
  struct stat filestatus;

  return (stat(name.c_str(), &filestatus) == 0);
}

std::string create_unique(const std::string &path,
                          const std::string &extension) {

  std::ostringstream tmp_stream;
  for (int i = 0; i < DIRECTORY_LAYERS; i++) {
    tmp_stream << std::internal << std::setfill('0')
               << std::setw(CHARS_PER_LAYER_NAME)
               << std::rand() % DIRECTORIES_PER_LAYER << "/";
  }

  std::string unique_id;
  std::string name;
  const char &last = path.back();

  do {
    uint64_t id = get_uint64();
    std::stringstream ss;
    ss << std::hex << id;
    unique_id = ss.str();
    name = path + std::string((last != '/') ? "/" : "") + tmp_stream.str() +
           unique_id + "." + extension;

  } while (exists(name));

  return name;
}
} // namespace VCL
