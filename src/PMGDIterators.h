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
}
