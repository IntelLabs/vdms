/**
 * @file   SearchExpression.h
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

#include <vector>
#include "jarvis.h"

/// Search expression to query a Jarvis Lake database
///
/// Search expression is currently limited to be the
/// conjunctions of an arbitrary number of Node's property
/// predicates.
///
/// Create the search expression by adding any number of
/// predicates.
///
/// Calling Eval() returns a node iterator.
class SearchExpression {
    Jarvis::StringID _tag;

    /// Opaque definition of a node iterator
    class SearchExpressionIterator;

    /// Opaque definition of an edge iterator
    class EdgeSearchExpressionIterator;

    /// The conjunctions of property predicates
    std::vector<Jarvis::PropertyPredicate> _predicates;

    /// A pointer to the database
    Jarvis::Graph &_db;

public:
    /// Construction requires a handle to a database
    SearchExpression(Jarvis::Graph &db, Jarvis::StringID tag) : _db(db), _tag(tag) {}

    void add(Jarvis::PropertyPredicate pp) { _predicates.push_back(pp); }
    const Jarvis::StringID tag() const { return _tag; };

    Jarvis::NodeIterator eval_nodes();
    Jarvis::NodeIterator eval_nodes(const Jarvis::Node &node, Jarvis::Direction dir = Jarvis::Any,
                                       Jarvis::StringID edgetag = 0, bool unique = true);

    Jarvis::EdgeIterator eval_edges();
};
