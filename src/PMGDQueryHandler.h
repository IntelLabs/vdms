#pragma once

#include <map>
#include <unordered_map>
#include <mutex>
#include <vector>

#include "protobuf/pmgdMessages.pb.h" // Protobuff implementation
#include "jarvis.h"
#include "SearchExpression.h"

namespace athena {
    // Instance created per worker thread to handle all transactions on a given
    // connection.
    class PMGDQueryHandler
    {
        class NewNodeIteratorImpl : public Jarvis::NodeIteratorImplIntf
        {
            Jarvis::Node &_n;
            bool start = true;

        public:
            NewNodeIteratorImpl(Jarvis::Node &n)
                : _n(n)
                { }

            operator bool() const { return start; }

            /// No next matching node
            bool next()
            {
                start = false;
                return start;
            }

            Jarvis::Node *ref() { return (start ? &_n : NULL); }
        };

        class MultiNeighborIteratorImpl : public Jarvis::NodeIteratorImplIntf
        {
            // Iterator for the starting nodes.
            Jarvis::NodeIterator *_start_ni;
            SearchExpression _search_neighbors;
            Jarvis::NodeIterator *_neighb_i;     // TODO: Does this work properly?
            Jarvis::Direction _dir;
            Jarvis::StringID _edge_tag;

            bool _next()
            {
                while (bool(*_start_ni)) {
                    if (_neighb_i != NULL)
                        delete _neighb_i;

                    // TODO Maybe unique can have a default value of false.
                    // TODO No support in case unique is true but get it from LDBC.
                    // Eventually need to add a get_union(NodeIterator, vector<Constraints>)
                    // call to PMGD.
                    // TODO Any way to skip new?
                    _neighb_i = new Jarvis::NodeIterator(_search_neighbors.eval_nodes(**_start_ni,
                                               _dir, _edge_tag));
                    _start_ni->next();
                    if (bool(*_neighb_i))
                        return true;
                }
                return false;
            }

        public:
            MultiNeighborIteratorImpl(Jarvis::NodeIterator *start_ni,
                                      SearchExpression search_neighbors,
                                      Jarvis::Direction dir,
                                      Jarvis::StringID edge_tag)
                : _start_ni(start_ni),
                  _search_neighbors(search_neighbors),
                  _neighb_i(NULL),
                  _dir(dir),
                  _edge_tag(edge_tag)
            {
                _next();
            }

            ~MultiNeighborIteratorImpl()
            {
                if (_neighb_i != NULL)
                    delete _neighb_i;
                _neighb_i = NULL;
            }

            operator bool() const { return _neighb_i != NULL ? bool(*_neighb_i) : false; }

            /// No next matching node
            bool next()
            {
                if (_neighb_i != NULL && bool(*_neighb_i)) {
                    _neighb_i->next();
                    if (bool(*_neighb_i))
                        return true;
                }
                return _next();
            }

            Jarvis::Node *ref() { return &**_neighb_i; }
        };

        // This class is instantiated by the server each time there is a new
        // connection. So someone needs to pass a handle to the graph db itself.
        Jarvis::Graph *_db;

        // Need this lock till we have concurrency support in JL
        // TODO: Make this reader writer.
        std::mutex *_dblock;

        Jarvis::Transaction *_tx;

        // Map an integer ID to a NodeIterator (reset at the end of each transaction).
        // This works for Adds and Queries. We assume that the client or
        // the request server code will always add a ref to the AddNode
        // call or a query call. That is what is the key in the map.
        // Add calls typically don't result in a NodeIterator. But we make
        // one to keep the code uniform. In a chained query, there is no way
        // of finding out if the reference is for an AddNode or a QueryNode
        // and rather than searching multiple maps, we keep it uniform here.
        // TODO this might have to map to a Global ID in the distributed setting.
        std::unordered_map<int, Jarvis::NodeIterator *> mNodes;

        // Map an integer ID to an Edge (reset at the end of each transaction).
        // TODO this might have to map to a Global ID in the distributed setting.
        // Not really used at this point.
        std::unordered_map<int, Jarvis::Edge *> mEdges;

        template <class Element> void set_property(Element &e, const pmgd::protobufs::Property &p);
        void add_node(const pmgd::protobufs::AddNode &cn, pmgd::protobufs::CommandResponse *response);
        void add_edge(const pmgd::protobufs::AddEdge &ce, pmgd::protobufs::CommandResponse *response);
        Jarvis::Property construct_search_property(const pmgd::protobufs::Property &p);
        Jarvis::PropertyPredicate construct_search_term(const pmgd::protobufs::PropertyPredicate &p_pp);
        void construct_protobuf_property(const Jarvis::Property &j_p, pmgd::protobufs::Property *p_p);
        void query_node(const pmgd::protobufs::QueryNode &qn, pmgd::protobufs::CommandResponse *response);
        // void query_neighbor(const pmgd::protobufs::QueryNeighbor &qnb, pmgd::protobufs::CommandResponse *response);
        void process_query(pmgd::protobufs::Command *cmd, pmgd::protobufs::CommandResponse *response);

    public:
        PMGDQueryHandler(Jarvis::Graph *db, std::mutex *mtx);

        // The vector here can contain just one JL command but will be surrounded by
        // TX begin and end. So just expose one call to the QueryHandler for
        // the request server.
        // The return vector contains an ordered list of query id with
        // group of commands that correspond to that query.
        // Pass the number of queries here since that could be different
        // than the number of commands.
        // Ensure that the cmd_grp_id, that is the query number are in increasing
        // order and account for the TxBegin and TxEnd in numbering.
        std::vector<std::vector<pmgd::protobufs::CommandResponse *>> process_queries(
                                           std::vector<pmgd::protobufs::Command *> cmds,
                                           int num_queries);
    };
};
