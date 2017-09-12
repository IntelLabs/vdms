#include "SearchExpression.h"
#include "jarvis.h"
#include "neighbor.h"

class SearchExpression::SearchExpressionIterator : public Jarvis::NodeIteratorImplIntf
{
    /// Reference to expression to evaluate
    const SearchExpression _expr;

    /// Node iterator on the first property predicate
    Jarvis::NodeIterator mNodeIt;

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
            for (std::size_t i = _start_at; i < _expr._predicates.size(); i++) {
                Jarvis::PropertyFilter<Jarvis::Node> pf(_expr._predicates.at(i));
                if (pf(*mNodeIt) == Jarvis::DontPass)
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
    SearchExpressionIterator(const SearchExpression &expr)
        : _expr(expr),
          mNodeIt(_expr._db.get_nodes(_expr.tag(),
                      (_expr._predicates.empty() ? Jarvis::PropertyPredicate()
                           : _expr._predicates.at(0)))),
          _neighbor(false)
    {
        _start_at = 1;
        _next();
    }

    /// Construct an iterator given the search expression for neighbors
    ///
    /// Postcondition: mNodeIt points to the first matching node, or
    /// returns NULL.
    SearchExpressionIterator(const Jarvis::Node &node, Jarvis::Direction dir,
                               Jarvis::StringID edgetag, bool unique,
                               const SearchExpression &neighbor_expr)
        : _expr(neighbor_expr),
          mNodeIt(get_neighbors(node, dir, edgetag, unique)),
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

    Jarvis::Node *ref() { return &*mNodeIt; }
};

// *** Could find a template way of combining Node and Edge iterator.
class SearchExpression::EdgeSearchExpressionIterator : public Jarvis::EdgeIteratorImplIntf
{
    /// Reference to expression to evaluate
    const SearchExpression &_expr;

    /// Node iterator on the first property predicate
    Jarvis::EdgeIterator mEdgeIt;

    /// Advance to the next matching node
    /// @returns true if we find a matching node
    /// Precondition: mNodeIt points to the next possible node
    /// candidate
    bool _next()
    {
        for (; mEdgeIt; mEdgeIt.next()) {
            for (std::size_t i = 1; i < _expr._predicates.size(); i++) {
                Jarvis::PropertyFilter<Jarvis::Edge> pf(_expr._predicates.at(i));
                if (pf(*mEdgeIt) == Jarvis::DontPass)
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
    EdgeSearchExpressionIterator(const SearchExpression &expr)
        : _expr(expr),
        mEdgeIt(_expr._db.get_edges(_expr.tag(),
                    (_expr._predicates.empty() ? Jarvis::PropertyPredicate()
                         : _expr._predicates.at(0))))
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

    Jarvis::EdgeRef *ref() { return &*mEdgeIt; }
    Jarvis::StringID get_tag() const { return mEdgeIt->get_tag(); }
    Jarvis::Node &get_source() const { return mEdgeIt->get_source(); }
    Jarvis::Node &get_destination() const { return mEdgeIt->get_destination(); }
    Jarvis::Edge *get_edge() const { return &static_cast<Jarvis::Edge &>(*mEdgeIt); }
};

/// Evaluate the associated search expression
/// @returns an iterator over the search expression
Jarvis::NodeIterator SearchExpression::eval_nodes()
{
    return Jarvis::NodeIterator(new SearchExpressionIterator(*this));
}

/// Evaluate the associated search expression on neighbors
/// @returns an iterator over the search expression
Jarvis::NodeIterator SearchExpression::eval_nodes(const Jarvis::Node &node, Jarvis::Direction dir,
                                                   Jarvis::StringID edgetag, bool unique)
{
    return Jarvis::NodeIterator(new SearchExpressionIterator(node, dir, edgetag, unique, *this));
}

/// Evaluate the associated search expression
/// @returns an iterator over the search expression
Jarvis::EdgeIterator SearchExpression::eval_edges()
{
    return Jarvis::EdgeIterator(new EdgeSearchExpressionIterator(*this));
}
