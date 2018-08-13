/**
 * @file   PMGDQueryHandler.h
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

#include <map>
#include <unordered_map>
#include <vector>
#include <list>

#include "protobuf/pmgdMessages.pb.h" // Protobuff implementation
#include "pmgd.h"

namespace VDMS {
    // Instance created per worker thread to handle all transactions on a given
    // connection.

    typedef PMGD::protobufs::Command           PMGDCmd;
    typedef PMGD::protobufs::PropertyPredicate PMGDPropPred;
    typedef PMGD::protobufs::PropertyList      PMGDPropList;
    typedef PMGD::protobufs::Property          PMGDProp;
    typedef PMGD::protobufs::Constraints       PMGDQueryConstraints;
    typedef PMGD::protobufs::ResultInfo        PMGDQueryResultInfo;
    typedef PMGD::protobufs::QueryNode         PMGDQueryNode;
    typedef PMGD::protobufs::CommandResponse   PMGDCmdResponse;
    typedef PMGD::protobufs::ResponseType      PMGDRespType;
    typedef PMGDCmdResponse::ErrorCode         PMGDCmdErrorCode;

    typedef std::vector<PMGDCmd *>             PMGDCmds;
    typedef std::vector<PMGDCmdResponse *>     PMGDCmdResponses;

    class RWLock;

    class PMGDQueryHandler
    {
        class ReusableNodeIterator;
        class MultiNeighborIteratorImpl;

        // Until we have a separate PMGD server this db lives here
        static PMGD::Graph *_db;

        // Need this lock till we have concurrency support in PMGD
        static RWLock *_dblock;

        PMGD::Transaction *_tx;
        bool _readonly;  // Variable changes per TX based on process_queries parameter.

        // Map an integer ID to a NodeIterator (reset at the end of each transaction).
        // This works for Adds and Queries. We assume that the client or
        // the request server code will always add a ref to the AddNode
        // call or a query call. That is what is the key in the map.
        // Add calls typically don't result in a NodeIterator. But we make
        // one to keep the code uniform. In a chained query, there is no way
        // of finding out if the reference is for an AddNode or a QueryNode
        // and rather than searching multiple maps, we keep it uniform here.
        std::unordered_map<int, ReusableNodeIterator *> _cached_nodes;

        int process_query(const PMGDCmd *cmd, PMGDCmdResponse *response);
        void error_cleanup(std::vector<PMGDCmdResponses> &responses, PMGDCmdResponse *last_resp);
        int add_node(const PMGD::protobufs::AddNode &cn, PMGDCmdResponse *response);
        int update_node(const PMGD::protobufs::UpdateNode &un, PMGDCmdResponse *response);
        int add_edge(const PMGD::protobufs::AddEdge &ce, PMGDCmdResponse *response);
        template <class Element> void set_property(Element &e, const PMGDProp&p);
        int query_node(const PMGDQueryNode &qn, PMGDCmdResponse *response);
        PMGD::PropertyPredicate construct_search_term(const PMGDPropPred &p_pp);
        PMGD::Property construct_search_property(const PMGDProp&p);
        template <class Iterator> void build_results(Iterator &ni,
                                                    const PMGDQueryResultInfo &qn,
                                                    PMGDCmdResponse *response);
        void construct_protobuf_property(const PMGD::Property &j_p, PMGDProp*p_p);
        void construct_missing_property(PMGDProp *p_p);

        void set_response(PMGDCmdResponse *response, PMGDCmdErrorCode error_code,
                            std::string error_msg)
        {
            response->set_error_code(error_code);
            response->set_error_msg(error_msg);
        }

        void set_response(PMGDCmdResponse *response, PMGDRespType type,
                            PMGDCmdErrorCode error_code)
        {
            response->set_r_type(type);
            response->set_error_code(error_code);
        }

        void set_response(PMGDCmdResponse *response, PMGDRespType type,
                            PMGDCmdErrorCode error_code, std::string error_msg)
        {
            response->set_r_type(type);
            response->set_error_code(error_code);
            response->set_error_msg(error_msg);
        }

    public:
        static void init();
        static void destroy();
        PMGDQueryHandler() { _tx = NULL; _readonly = true; }

        // The vector here can contain just one JL command but will be surrounded by
        // TX begin and end. So just expose one call to the QueryHandler for
        // the request server.
        // The return vector contains an ordered list of query id with
        // group of commands that correspond to that query.
        // Pass the number of queries here since that could be different
        // than the number of commands.
        // Ensure that the cmd_grp_id, that is the query number are in increasing
        // order and account for the TxBegin and TxEnd in numbering.
        std::vector<PMGDCmdResponses> process_queries(const PMGDCmds &cmds,
                                                      int num_groups, bool readonly);
    };
};
