/**
 * @file   SearchExpression.cc
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

#include "SearchExpression.h"
#include "pmgd.h"
#include "neighbor.h"

using namespace VDMS;

class SearchExpression::NodeAndIteratorImpl : public PMGD::NodeIteratorImplIntf
{
    /// Reference to expression to evaluate
    const SearchExpression _expr;

    /// Node iterator on the first property predicate
    PMGD::NodeIterator mNodeIt;

    // Indicate where to start in the search expression vector
    unsigned _start_at;

    // Indicate if it is a neighbor search
    bool _neighbor;

    /// Advance to the next matching node
    /// @returns true if we find a matching node
    /// Precondition: mNodeIt points to the next possible node
    /// candidate
    bool _next()
    {
        for (; mNodeIt; mNodeIt.next()) {
            if (_neighbor && (_expr.tag() != 0 && mNodeIt->get_tag() != _expr.tag()) )
                goto continueNodeIt;
            for (std::size_t i = _start_at; i < _expr._node_predicates.size(); i++) {
                PMGD::PropertyFilter<PMGD::Node> pf(_expr._node_predicates.at(i));
                if (pf(*mNodeIt) == PMGD::DontPass)
                    goto continueNodeIt;
            }
            return true;
        continueNodeIt:;
        }
        return false;
    }

public:
    /// Construct an iterator given the search expression
    ///
    /// Postcondition: mNodeIt points to the first matching node, or
    /// returns NULL.
    NodeAndIteratorImpl(const SearchExpression &expr)
        : _expr(expr),
          mNodeIt(_expr._db.get_nodes(_expr.tag(),
                      (_expr._node_predicates.empty() ? PMGD::PropertyPredicate()
                           : _expr._node_predicates.at(0)))),
          _neighbor(false)
    {
        _start_at = 1;
        _next();
    }

    /// Construct an iterator given the search expression for neighbors
    ///
    /// Postcondition: mNodeIt points to the first matching node, or
    /// returns NULL.
    NodeAndIteratorImpl(const PMGD::Node &node, PMGD::Direction dir,
                               PMGD::StringID edgetag, bool unique,
                               const SearchExpression &neighbor_expr)
        : _expr(neighbor_expr),
          mNodeIt(get_neighbors(node, dir, edgetag,
                                _expr.get_edge_predicates(), unique)),
          _neighbor(true)
    {
        _start_at = 0;
        _next();
    }

    operator bool() const { return bool(mNodeIt); }

    /// Advance to the next node
    /// @returns true if such a next node exists
    bool next()
    {
        mNodeIt.next();
        return _next();
    }

    PMGD::Node *ref() { return &*mNodeIt; }
};

class SearchExpression::NodeOrIteratorImpl : public PMGD::NodeIteratorImplIntf
{
    /// Reference to expression to evaluate
    const SearchExpression _expr;

    /// Node iterator on the first property predicate
    PMGD::Node* _node;

    // Indicate where to start in the search expression vector
    unsigned _idx;

    // Indicate if it is a neighbor search
    bool _neighbor;

    PMGD::NodeIterator _neighborIt;

    /// Advance to the next matching node
    /// @returns true if we find a matching node
    /// Precondition: _node points to the next possible node
    /// candidate
    bool _next()
    {
        while (_idx < _expr._node_predicates.size()) {
             PMGD::NodeIterator ni =
                    _expr._db.get_nodes(_expr.tag(),
                                        _expr._node_predicates.at(_idx++));

            if (ni) {
                _node = &*ni;
                return true;
            }
        }

        return false;
    }

    bool _next_neighbor()
    {
        static int id = 0;
        while (_neighborIt) {
            for (const auto& pred : _expr._node_predicates) {
                PMGD::PropertyFilter<PMGD::Node> pf(pred);
                if (pf(*_neighborIt) == PMGD::Pass) {
                    _node = &*_neighborIt;
                    return true;
                }
            }

            _neighborIt.next();
        }

        return false;
    }

public:
    /// Construct an iterator given the search expression
    ///
    /// Postcondition: _node points to the first matching node, or
    /// returns NULL.
    NodeOrIteratorImpl(const SearchExpression &expr)
        : _expr(expr),
          _idx(0),
          _neighbor(false),
          _neighborIt(NULL)
    {
        _next();
    }

    /// Construct an iterator given the search expression for neighbors
    ///
    /// Postcondition: _node points to the first matching node, or
    /// returns NULL.
    NodeOrIteratorImpl(const PMGD::Node &node, PMGD::Direction dir,
                       PMGD::StringID edgetag, bool unique,
                       const SearchExpression &neighbor_expr)
        : _expr(neighbor_expr),
          _neighborIt(get_neighbors(node, dir, edgetag,
                                    _expr.get_edge_predicates(), unique)),
          _neighbor(true)
    {
        _next_neighbor();
    }

    operator bool() const { return bool(_node); }

    /// Advance to the next node
    /// @returns true if such a next node exists
    bool next()
    {
        if (_neighbor) {
            _neighborIt.next();
            return _next_neighbor();
        }
        else {
            return _next();
        }
    }

    PMGD::Node *ref() { return _node; }
};

// *** Could find a template way of combining Node and Edge iterator.
class SearchExpression::EdgeAndIteratorImpl : public PMGD::EdgeIteratorImplIntf
{
    /// Reference to expression to evaluate
    const SearchExpression &_expr;

    /// Node iterator on the first property predicate
    PMGD::EdgeIterator mEdgeIt;

    /// Advance to the next matching node
    /// @returns true if we find a matching node
    /// Precondition: mNodeIt points to the next possible node
    /// candidate
    bool _next()
    {
        for (; mEdgeIt; mEdgeIt.next()) {
            for (std::size_t i = 1; i < _expr._node_predicates.size(); i++) {
                PMGD::PropertyFilter<PMGD::Edge> pf(_expr._node_predicates.at(i));
                if (pf(*mEdgeIt) == PMGD::DontPass)
                    goto continueEdgeIt;
            }
            return true;
        continueEdgeIt:;
        }
        return false;
    }

public:
    /// Construct an iterator given the search expression
    ///
    /// Postcondition: mEdgeIt points to the first matching edge, or
    /// returns NULL.
    EdgeAndIteratorImpl(const SearchExpression &expr)
        : _expr(expr),
        mEdgeIt(_expr._db.get_edges(_expr.tag(),
                    (_expr._node_predicates.empty() ? PMGD::PropertyPredicate()
                         : _expr._node_predicates.at(0))))
    {
        _next();
    }

    operator bool() const { return bool(mEdgeIt); }

    /// Advance to the next node
    /// @returns true if such a next node exists
    bool next()
    {
        mEdgeIt.next();
        return _next();
    }

    PMGD::EdgeRef *ref() { return &*mEdgeIt; }
    PMGD::StringID get_tag() const { return mEdgeIt->get_tag(); }
    PMGD::Node &get_source() const { return mEdgeIt->get_source(); }
    PMGD::Node &get_destination() const { return mEdgeIt->get_destination(); }
    PMGD::Edge *get_edge() const { return &static_cast<PMGD::Edge &>(*mEdgeIt); }
};

/// Evaluate the associated search expression
/// @returns an iterator over the search expression
PMGD::NodeIterator SearchExpression::eval_nodes()
{
    if (_or)
        return PMGD::NodeIterator(new NodeOrIteratorImpl(*this));
    else
        return PMGD::NodeIterator(new NodeAndIteratorImpl(*this));
}

/// Evaluate the associated search expression on neighbors
/// @returns an iterator over the search expression
PMGD::NodeIterator SearchExpression::eval_nodes
    (const PMGD::Node &node, PMGD::Direction dir,
     PMGD::StringID edgetag, bool unique)
{
    if (_or)
        return PMGD::NodeIterator(new NodeOrIteratorImpl(node, dir, edgetag, unique, *this));
    else
        return PMGD::NodeIterator(new NodeAndIteratorImpl(node, dir, edgetag, unique, *this));
}

/// Evaluate the associated search expression
/// @returns an iterator over the search expression
PMGD::EdgeIterator SearchExpression::eval_edges()
{
    return PMGD::EdgeIterator(new EdgeAndIteratorImpl(*this));
}
