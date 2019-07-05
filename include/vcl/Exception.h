/**
 * @file   Exception.h
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
 * @section DESCRIPTION
 *
 */

#pragma once

#include <string>

namespace VCL {
    enum ExceptionType {
      UndefinedException,

      UnsupportedFormat,
      UnsupportedOperation,
      UnsupportedIndex,

      ObjectNotFound,
      OpenFailed,
      NotImplemented,

      ObjectEmpty,

      SizeMismatch,
      OutOfBounds,

      TileDBNotFound,
      TileDBError,

      OpenCVError,

      FFmpegInitFailed,
      FFmpegParseFailed,

    };

    struct Exception {
      // Which exception
      int num;            ///< Exception number
      const char *name;   ///< Exception name

      // Additional information
      std::string msg;
      int errno_val;

      // Where it was thrown
      const char *file;   ///< Source file name
      int line;           ///< Source line number

      Exception(int exc, const char *exc_name, const char *f, int l)
          : num(exc), name(exc_name),
            msg(), errno_val(0),
            file(f), line(l)
      {}

      Exception(int exc, const char *exc_name,
                  const std::string &m,
                  const char *f, int l)
            : num(exc), name(exc_name),
              msg(m), errno_val(0),
              file(f), line(l)
        {}

      Exception(int exc, const char *exc_name,
                  int err, const std::string &m,
                  const char *f, int l)
            : num(exc), name(exc_name),
              msg(m), errno_val(err),
              file(f), line(l)
        {}
    };

#define VCLException(name, ...) \
    VCL::Exception(VCL::name, #name, ##__VA_ARGS__, __FILE__, __LINE__)
};

extern void print_exception(const VCL::Exception &e, FILE *f= stdout);
