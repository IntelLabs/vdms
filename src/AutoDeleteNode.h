/**
 * @file   AutoDeleteNode.h
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

#include <iostream>
#include <stdio.h>

#include <queue>
#include <vector>

#include <jsoncpp/json/json.h>
#include <jsoncpp/json/value.h>

#include "iterator.h"
#include "node.h"

class AutoDeleteNode {
private:
  Json::UInt64 _expiration_timestamp;
  void *_node; // can use void pointer because query only seraches for Nodes and
               // not edges
public:
  AutoDeleteNode(Json::UInt64 new_expiration_timestamp, void *n_node);
  ~AutoDeleteNode();
  Json::UInt64 GetExpirationTimestamp();
  void *GetNode();
};

struct GreaterThanTimestamp {
  bool operator()(AutoDeleteNode *lhs, AutoDeleteNode *rhs) const {
    return lhs->GetExpirationTimestamp() > rhs->GetExpirationTimestamp();
  }
};
