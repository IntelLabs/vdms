/**
 * @file   Neo4jQueryHelpers.h
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2024 Intel Corporation
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
#include <jsoncpp/json/value.h>

/*Hat Tip
 * https://stackoverflow.com/questions/4800605/iterating-through-objects-in-jsoncpp*/
void PrintJSONValue(const Json::Value &val) {
  if (val.isString()) {
    printf("string(%s)", val.asString().c_str());
  } else if (val.isBool()) {
    printf("bool(%d)", val.asBool());
  } else if (val.isInt()) {
    printf("int(%d)", val.asInt());
  } else if (val.isUInt()) {
    printf("uint(%u)", val.asUInt());
  } else if (val.isDouble()) {
    printf("double(%f)", val.asDouble());
  } else {
    printf("unknown type=[%d]", val.type());
  }
}

void json_printer(const Json::Value &root, unsigned short depth) {

  printf("\n---------JSON Dumper-----------\n");
  for (Json::Value::const_iterator itr = root.begin(); itr != root.end();
       itr++) {
    printf(" subvalue(");
    PrintJSONValue(itr.key());
    printf(") -");
    json_printer(*itr, depth);
  }
}
