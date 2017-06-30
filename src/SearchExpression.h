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
    Jarvis::StringID &_tag;

    /// Opaque definition of a node iterator
    class SearchExpressionIterator;

    /// Opaque definition of an edge iterator
    class EdgeSearchExpressionIterator;

    /// The conjunctions of property predicates
    std::vector<Jarvis::PropertyPredicate> mExpr;

    /// A pointer to the database
    Jarvis::Graph &mDB;

public:
    /// Construction requires a handle to a database
    SearchExpression(Jarvis::Graph &db, Jarvis::StringID tag) : mDB(db), _tag(tag) {}

    void Add(Jarvis::PropertyPredicate pp) { mExpr.push_back(pp); }
    const Jarvis::StringID &Tag() const { return _tag; }

    Jarvis::NodeIterator EvalNodes();
    Jarvis::NodeIterator EvalNodes(const Jarvis::Node &node, Jarvis::Direction dir = Jarvis::Any,
                                       Jarvis::StringID edgetag = 0, bool unique = true);

    Jarvis::EdgeIterator EvalEdges();
};
