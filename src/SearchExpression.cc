#include "SearchExpression.h"
#include "jarvis.h"

class SearchExpression::SearchExpressionIterator : public Jarvis::NodeIteratorImplIntf
{
    /// Reference to expression to evaluate
    const SearchExpression &mExpr;

    /// Node iterator on the first property predicate
    Jarvis::NodeIterator mNodeIt;

    /// Advance to the next matching node
    /// @returns true if we find a matching node
    /// Precondition: mNodeIt points to the next possible node
    /// candidate
    bool _next()
    {
        for (; mNodeIt; mNodeIt.next()) {
            for (std::size_t i = 1; i < mExpr.mExpr.size(); i++) {
                Jarvis::PropertyFilter<Jarvis::Node> pf(mExpr.mExpr.at(i));
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
        : mExpr(expr),
          mNodeIt(mExpr.mDB.get_nodes(mExpr.Tag(), (mExpr.mExpr.empty() ? Jarvis::PropertyPredicate() : mExpr.mExpr.at(0))))
    {
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
    const SearchExpression &mExpr;

    /// Node iterator on the first property predicate
    Jarvis::EdgeIterator mEdgeIt;

    /// Advance to the next matching node
    /// @returns true if we find a matching node
    /// Precondition: mNodeIt points to the next possible node
    /// candidate
    bool _next()
    {
        for (; mEdgeIt; mEdgeIt.next()) {
            for (std::size_t i = 1; i < mExpr.mExpr.size(); i++) {
                Jarvis::PropertyFilter<Jarvis::Edge> pf(mExpr.mExpr.at(i));
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
        : mExpr(expr),
        mEdgeIt(mExpr.mDB.get_edges(mExpr.Tag(), (mExpr.mExpr.empty() ? Jarvis::PropertyPredicate() : mExpr.mExpr.at(0))))
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
Jarvis::NodeIterator SearchExpression::EvalNodes()
{
    return Jarvis::NodeIterator(new SearchExpressionIterator(*this));
}

/// Evaluate the associated search expression
/// @returns an iterator over the search expression
Jarvis::EdgeIterator SearchExpression::EvalEdges()
{
    return Jarvis::EdgeIterator(new EdgeSearchExpressionIterator(*this));
}