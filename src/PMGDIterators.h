/**
 * @file   PMGDIterators.h
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

#include <unordered_set>

#include "pmgd.h"
#include "PMGDQueryHandler.h"
#include "SearchExpression.h"

namespace VDMS {
    class PMGDQueryHandler::ReusableNodeIterator
    {
        // Iterator for the starting nodes.
        PMGD::NodeIterator _ni;

        // TODO Is list the best data structure if we could potentially
        // sort?
        std::list<PMGD::Node *> _traversed;

        // Current postion of list iterator
        std::list<PMGD::Node *>::iterator _it;

        bool _next();

        PMGD::Node *ref()
        {
            if (!bool(*this))
                throw PMGDException(NullIterator, "Null impl");
            return *_it;
        }

        // TODO Is this the best way to do this
        struct compare_propkey
        {
            PMGD::StringID _propid;
            bool operator()(const PMGD::Node *n1, const PMGD::Node *n2)
              { return n1->get_property(_propid) < n2->get_property(_propid); }
        };

    public:
        // Make sure this is not auto-declared. The move one won't be.
        ReusableNodeIterator(const ReusableNodeIterator &) = delete;
        ReusableNodeIterator(PMGD::NodeIterator ni)
            : _ni(ni),
            _it(_traversed.begin())
        { _next(); }

        // Add this to clean up the NewNodeIterator requirement
        ReusableNodeIterator(PMGD::Node *n)
            : _ni(NULL),
            _it(_traversed.insert(_traversed.end(), n))
        {}

        operator bool() const { return _it != _traversed.end(); }
        bool next() { return _next(); }
        PMGD::Node &operator *() { return *ref(); }
        PMGD::Node *operator ->() { return ref(); }
        void reset() { _it = _traversed.begin(); }
        void traverse_all()
        {
            for( ; _ni; _ni.next())
                _traversed.insert(_traversed.end(), &*_ni);
        }

        // Sort the list. Once the list is sorted, all operations
        // following that happen in a sorted manner. And this function
        // resets the iterator to the beginning.
        void sort(PMGD::StringID sortkey);
    };

    class PMGDQueryHandler::MultiNeighborIteratorImpl :
                            public PMGD::NodeIteratorImplIntf
    {
        // Iterator for the starting nodes.
        ReusableNodeIterator *_start_ni;
        SearchExpression _search_neighbors;
        PMGD::NodeIterator *_neighb_i;
        PMGD::Direction _dir;
        PMGD::StringID _edge_tag;

        bool _next();

    public:
        MultiNeighborIteratorImpl(ReusableNodeIterator *start_ni,
                SearchExpression search_neighbors,
                PMGD::Direction dir,
                PMGD::StringID edge_tag)
            : _start_ni(start_ni),
            _search_neighbors(search_neighbors),
            _neighb_i(NULL),
            _dir(dir),
            _edge_tag(edge_tag)
        { _next(); }

        ~MultiNeighborIteratorImpl()
        {
            delete _neighb_i;
        }

        operator bool() const { return _neighb_i != NULL ? bool(*_neighb_i) : false; }

        /// No next matching node
        bool next();

        PMGD::Node *ref() { return &**_neighb_i; }
    };

    class PMGDQueryHandler::NodeEdgeIteratorImpl : public PMGD::EdgeIteratorImplIntf
    {
        /// Reference to expression to evaluate
        const SearchExpression _expr;
        const size_t _num_predicates;

        ReusableNodeIterator *_src_ni;
        ReusableNodeIterator *_dest_ni;

        // In order to check if the other end of an edge is in the nodes
        // covered by the dest_ni, it is best to store those nodes in an
        // easily searchable data structure, which a list inside ReusableNodeIterator
        // is not. Besides, it doesn't make sense to expose that list here.
        std::unordered_set<PMGD::Node *> _dest_nodes;

        std::size_t _pred_start;
        PMGD::Direction _dir;
        bool _check_dest;

        PMGD::EdgeIterator *_edge_it;

        bool _next();
        bool check_predicates();

        PMGD::EdgeIterator return_iterator()
        {
            _dir = PMGD::Direction::Outgoing;
            if (_src_ni == NULL) {
                if (_dest_ni == NULL)
                    _pred_start = 1;
                else {
                    _dir = PMGD::Direction::Incoming;
                    _src_ni = _dest_ni;
                    _dest_ni = NULL;
                }
            }
            return (_src_ni == NULL || !bool(*_src_ni)) ? _expr.db().get_edges(_expr.tag(),
                        (_num_predicates <= 0 ? PMGD::PropertyPredicate()
                             : _expr.predicate(0))) :
                        (*_src_ni)->get_edges(_dir, _expr.tag());
        }

    public:
        NodeEdgeIteratorImpl(const SearchExpression &expr,
                             ReusableNodeIterator *src_ni,
                             ReusableNodeIterator *dest_ni = NULL)
            : _expr(expr), _num_predicates(_expr.num_predicates()),
              _src_ni(src_ni), _dest_ni(dest_ni),
              _pred_start(0), _check_dest(false),
              _edge_it(new PMGD::EdgeIterator(return_iterator()))
        {
            if (_dest_ni != NULL) {
                for (; bool(*_dest_ni); _dest_ni->next())
                    _dest_nodes.insert(&(**_dest_ni));
                // This iterator will be reset outside
                _dest_ni = NULL;
                _check_dest = true;
            }
            if (!check_predicates())
                next();
        }

        operator bool() const { return bool(*_edge_it); }

        bool next();
        PMGD::EdgeRef *ref() { return &(**_edge_it); }
        PMGD::StringID get_tag() const { return (*_edge_it)->get_tag(); }
        PMGD::Node &get_source() const { return (*_edge_it)->get_source(); }
        PMGD::Node &get_destination() const { return (*_edge_it)->get_destination(); }
        PMGD::Edge *get_edge() const { return &static_cast<PMGD::Edge &>(**_edge_it); }
    };

    class PMGDQueryHandler::ReusableEdgeIterator
    {
        // Iterator for the starting nodes.
        PMGD::EdgeIterator _ei;

        // TODO Is list the best data structure if we could potentially
        // sort?
        std::list<PMGD::Edge *> _traversed;

        // Current postion of list iterator
        std::list<PMGD::Edge *>::iterator _it;

        bool _next();

        PMGD::Edge *ref()
        {
            if (!bool(*this))
                throw PMGDException(NullIterator, "Null edge impl");
            return *_it;
        }

        // TODO Is this the best way to do this
        struct compare_propkey
        {
            PMGD::StringID _propid;
            bool operator()(const PMGD::Edge *e1, const PMGD::Edge *e2)
              { return e1->get_property(_propid) < e2->get_property(_propid); }
        };

    public:
        // Make sure this is not auto-declared. The move one won't be.
        ReusableEdgeIterator(const ReusableEdgeIterator &) = delete;
        ReusableEdgeIterator(PMGD::EdgeIterator &ei)
            : _ei(ei),
              _it(_traversed.begin())
          { _next(); }

        ReusableEdgeIterator(PMGD::Edge *e)
            : _ei(NULL),
            _it(_traversed.insert(_traversed.end(), e))
          {}

        // Add this to support adding of edges while traversing in add_edge
        ReusableEdgeIterator() : _ei(NULL), _it(_traversed.end())
          {}

        operator bool() const { return _it != _traversed.end(); }
        bool next() { return _next(); }
        PMGD::Edge &operator *() { return *ref(); }
        PMGD::Edge *operator ->() { return ref(); }
        void reset() { _it = _traversed.begin(); }
        void traverse_all()
        {
            for( ; _ei; _ei.next()) {
                _traversed.insert(_traversed.end(), &static_cast<PMGD::Edge &>(*_ei));
            }
        }

        // Allow adding of edges as we construct this iterator in add_edge
        // call. This is different than add_node since once add_edge can
        // cause multiple edges to be created depending on how many nodes
        // matched the source/destination conditions
        void add_edge(PMGD::Edge *e)
        {
            // Easiest to add to the end of list. If we are in middle of
            // traversal, then this edge might get skipped. Use this function
            // with that understanding ***
            _traversed.insert(_traversed.end(), e);
        }

        // Sort the list. Once the list is sorted, all operations
        // following that happen in a sorted manner. And this function
        // resets the iterator to the beginning.
        void sort(PMGD::StringID sortkey);
    };
}
