/**
 * @file   Exceptions.h
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2017 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell
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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#pragma once

#include <string>

namespace VDMS {

enum ExceptionServerType {
  FATAL_Server_Error,

  SignalHandler,
  NullConnection,

  Undefined = 100, // Any undefined error
};

struct ExceptionServer {
  // Which exception
  int num;          ///< Exception number
  const char *name; ///< Exception name

  // Additional information
  std::string msg;
  int errno_val;

  // Where it was thrown
  const char *file; ///< Source file name
  int line;         ///< Source line number

  ExceptionServer(int exc, const char *exc_name, const char *f, int l)
      : num(exc), name(exc_name), msg(), errno_val(0), file(f), line(l) {}

  ExceptionServer(int exc, const char *exc_name, const std::string &m,
                  const char *f, int l)
      : num(exc), name(exc_name), msg(m), errno_val(0), file(f), line(l) {}

  ExceptionServer(int exc, const char *exc_name, int err, const std::string &m,
                  const char *f, int l)
      : num(exc), name(exc_name), msg(m), errno_val(err), file(f), line(l) {}
};

#define ExceptionServer(name, ...)                                             \
  ExceptionServer(VDMS::name, #name, ##__VA_ARGS__, __FILE__, __LINE__)
}; // namespace VDMS

extern void print_exception(const VDMS::ExceptionServer &e, FILE *f = stdout);
