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
#include "pmgd.h"

/// Search expression to query a PMGD Lake database
///
/// Search expression is currently limited to be the
/// conjunctions of an arbitrary number of Node's property
/// predicates.
///
/// Create the search expression by adding any number of
/// predicates.
///
/// Calling Eval() returns a node iterator.
namespace VDMS {

    class SearchExpression
    {
        PMGD::StringID _tag;

        /// Opaque definition of a node iterator
        class NodeAndIteratorImpl;
        class NodeOrIteratorImpl;

        /// Opaque definition of an edge iterator
        class EdgeAndIteratorImpl;

        bool _or;

        /// The conjunctions of property predicates
        std::vector<PMGD::PropertyPredicate> _node_predicates;

        /// The conjunctions of property predicates for edges
        std::vector<PMGD::PropertyPredicate> _edge_predicates;

        /// A pointer to the database
        PMGD::Graph &_db;

    public:
        /// Construction requires a handle to a database
        SearchExpression(PMGD::Graph &db, PMGD::StringID tag, bool p_or) :
            _db(db), _tag(tag), _or(p_or) {}

        PMGD::Graph &db() const { return _db; }
        const PMGD::StringID tag() const { return _tag; };

        void add_node_predicate(PMGD::PropertyPredicate pp) {
                _node_predicates.push_back(pp); }
        const PMGD::PropertyPredicate &get_node_predicate(int i) const {
                return _node_predicates.at(i); }
        const size_t num_node_predicates() const {
                return _node_predicates.size(); }

        void add_edge_predicate(PMGD::PropertyPredicate pp) {
                _edge_predicates.push_back(pp); }
        const std::vector<PMGD::PropertyPredicate>& get_edge_predicates() const {
                return _edge_predicates; }

        PMGD::NodeIterator eval_nodes();
        PMGD::NodeIterator eval_nodes(const PMGD::Node &node,
                                      PMGD::Direction dir = PMGD::Any,
                                      PMGD::StringID edgetag = 0,
                                      bool unique = true);

        PMGD::EdgeIterator eval_edges();
    };

}; // end VDMS namespace
