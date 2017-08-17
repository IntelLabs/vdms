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
