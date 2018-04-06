/**
 * @file   PMGDQueryHandler.cc
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

#include <limits>
#include "VDMSConfig.h"
#include "PMGDQueryHandler.h"
#include "util.h"   // PMGD util
#include "PMGDIterators.h"
#include "RWLock.h"

// TODO In the complete version of VDMS, this file will live
// within PMGD which would replace the PMGD namespace. Some of
// these code pieces are temporary.
using namespace PMGD;
using namespace VDMS;

PMGD::Graph *PMGDQueryHandler::_db;
RWLock *PMGDQueryHandler::_dblock;

void PMGDQueryHandler::init()
{
    std::string dbname = VDMSConfig::instance()
                        ->get_string_value("pmgd_path", "default_pmgd");
    unsigned attempts = VDMSConfig::instance()
                        ->get_int_value("max_lock_attempts", RWLock::MAX_ATTEMPTS);
 
    // Create a db
    _db = new PMGD::Graph(dbname.c_str(), PMGD::Graph::Create);

    // Create the query handler here assuming database is valid now.
    _dblock = new RWLock(attempts);
}

void PMGDQueryHandler::destroy()
{
    if (_db) {
        delete _db;
        delete _dblock;
        _db = NULL;
        _dblock = NULL;
    }
}

std::vector<PMGDCmdResponses>
              PMGDQueryHandler::process_queries(const PMGDCmds &cmds,
              int num_groups, bool readonly)
{
    std::vector<PMGDCmdResponses> responses(num_groups);

    assert(_tx == NULL);

    // Assuming one query handler handles one TX at a time.
    _readonly = readonly;
    try {
        if (_readonly)
            _dblock->read_lock();
        else
            _dblock->write_lock();
    }
    catch (Exception e) {
        PMGDCmdResponses &resp_v = responses[0];
        PMGDCmdResponse *response = new PMGDCmdResponse();
        set_response(response, PMGDCmdResponse::Exception,
                        e.name + std::string(": ") +  e.msg);
        resp_v.push_back(response);
        return responses;
    }

    for (const auto cmd : cmds) {
        PMGDCmdResponse *response = new PMGDCmdResponse();
        if (process_query(cmd, response) < 0) {
            error_cleanup(responses, response);
            break;  // Goto cleanup site.
        }
        PMGDCmdResponses &resp_v = responses[cmd->cmd_grp_id()];
        resp_v.push_back(response);
    }

    // Delete the Reusable iterators here.
    for (auto it = _cached_nodes.begin(); it != _cached_nodes.end(); ++it) {
        if (it->second != NULL)
            delete it->second;
    }
    _cached_nodes.clear();
    if (_tx != NULL) {
        delete _tx;
        _tx = NULL;
    }

    if (_readonly)
        _dblock->read_unlock();
    else
        _dblock->write_unlock();
    return responses;
}

void PMGDQueryHandler::error_cleanup(std::vector<PMGDCmdResponses> &responses,
                                PMGDCmdResponse *last_resp)
{
    int num_groups = responses.size();
    for (unsigned i = 0; i < num_groups; ++i) {
        unsigned size = responses[i].size();
        for (unsigned j = 0; j < size; ++j) {
            if (responses[i][j] != NULL)
                delete responses[i][j];
        }
        responses[i].clear();
    }
    responses.clear();

    // Since we have shortened the container, this reference will still remain
    // valid. So we can reuse the 0th spot.
    last_resp->set_cmd_grp_id(0);
    PMGDCmdResponses resp_v1;
    resp_v1.push_back(last_resp);
    responses.push_back(resp_v1);
}

int PMGDQueryHandler::process_query(const PMGDCmd *cmd,
                                     PMGDCmdResponse *response)
{
    int retval = 0;
    try {
        int code = cmd->cmd_id();
        response->set_cmd_grp_id(cmd->cmd_grp_id());
        switch (code) {
            case PMGDCmd::TxBegin:
            {
                int tx_options = _readonly ? Transaction::ReadOnly : Transaction::ReadWrite;
                _tx = new Transaction(*_db, tx_options);
                set_response(response, protobufs::TX, PMGDCmdResponse::Success);
                break;
            }
            case PMGDCmd::TxCommit:
            {
                _tx->commit();
                set_response(response, protobufs::TX, PMGDCmdResponse::Success);
                break;
            }
            case PMGDCmd::TxAbort:
            {
                set_response(response, protobufs::TX, PMGDCmdResponse::Abort,
                                "Abort called");
                retval = -1;
                break;
            }
            case PMGDCmd::AddNode:
                retval = add_node(cmd->add_node(), response);
                break;
            case PMGDCmd::AddEdge:
                retval = add_edge(cmd->add_edge(), response);
                break;
            case PMGDCmd::QueryNode:
                retval = query_node(cmd->query_node(), response);
                break;
            case PMGDCmd::UpdateNode:
                update_node(cmd->update_node(), response);
                break;
        }
    }
    catch (Exception e) {
        set_response(response, PMGDCmdResponse::Exception,
                        e.name + std::string(": ") +  e.msg);
        retval = -1;
    }

    return retval;
}

int PMGDQueryHandler::add_node(const protobufs::AddNode &cn,
                                  PMGDCmdResponse *response)
{
    long id = cn.identifier();
    if (id >= 0 && _cached_nodes.find(id) != _cached_nodes.end()) {
        set_response(response, PMGDCmdResponse::Error, "Reuse of _ref value\n");
        return -1;
    }

    if (cn.has_query_node()) {
        query_node(cn.query_node(), response);

        // If we found the node we needed and it is unique, then this
        // is the expected response. Just change the error code to exists
        // as expected by an add_node return instead of Success as done
        // in usual query_node. If we were supposed to cache
        // the result, it should be done already.
        if (response->r_type() == protobufs::NodeID &&
                response->error_code() == PMGDCmdResponse::Success) {
            response->set_error_code(PMGDCmdResponse::Exists);
            return 0;
        }

        // The only situation where we would have to take further
        // action is if the iterator was empty. And if there was some
        // error like !unique, then we need to return the response as is.
        if (response->error_code() != PMGDCmdResponse::Empty)
            return -1;
    }

    // Since the node wasn't found, now add it.
    StringID sid(cn.node().tag().c_str());
    Node &n = _db->add_node(sid);
    if (id >= 0)
        _cached_nodes[id] = new ReusableNodeIterator(&n);

    for (int i = 0; i < cn.node().properties_size(); ++i) {
        const PMGDProp &p = cn.node().properties(i);
        set_property(n, p);
    }

    set_response(response, protobufs::NodeID, PMGDCmdResponse::Success);

    // TODO: Partition code goes here
    // For now, fill in the single system node id
    response->set_op_int_value(_db->get_id(n));
    return 0;
}

int PMGDQueryHandler::update_node(const protobufs::UpdateNode &un,
                    protobufs::CommandResponse *response)
{
    long id = un.identifier();
    bool query = un.has_query_node();

    // Make sure there is either an id or a query node
    if (id < 0 && !query) {
        set_response(response, PMGDCmdResponse::Error, "No way to find update node\n");
        return -1;
    }

    auto it = _cached_nodes.find(id);
    if (it == _cached_nodes.end()) {
        if (!query) {
            set_response(response, PMGDCmdResponse::Error, "Undefined _ref value used in update\n");
            return -1;
        }
        else {
            query_node(un.query_node(), response);
            if (response->error_code() != PMGDCmdResponse::Success)
                return -1;
            it = _cached_nodes.find(un.query_node().identifier());  // id could have been -ve
        }
    }

    auto nit = it->second;
    long updated = 0;
    for ( ; *nit; nit->next()) {
        Node &n = **nit;
        updated++;
        for (int i = 0; i < un.properties_size(); ++i) {
            const protobufs::Property &p = un.properties(i);
            set_property(n, p);
        }
        for (int i = 0; i < un.remove_props_size(); ++i)
            n.remove_property(un.remove_props(i).c_str());
    }
    nit->reset();
    set_response(response, protobufs::Count, PMGDCmdResponse::Success);
    response->set_op_int_value(updated);
    return 0;
}

int PMGDQueryHandler::add_edge(const protobufs::AddEdge &ce,
                                  PMGDCmdResponse *response)
{
    // Presumably this node gets placed here.
    StringID sid(ce.edge().tag().c_str());

    // Assumes there could be multiple.
    ReusableNodeIterator *srcni, *dstni;

    // Since _ref is optional, need to make sure the map has the
    // right reference.
    auto srcit = _cached_nodes.find(ce.edge().src());
    auto dstit = _cached_nodes.find(ce.edge().dst());
    if (srcit != _cached_nodes.end() && dstit != _cached_nodes.end()) {
        srcni = srcit->second;
        dstni = dstit->second;
    }
    else {
        set_response(response, PMGDCmdResponse::Error,
                        "Source/destination node references not found");
        return -1;
    }

    if (srcni == NULL || dstni == NULL || !bool(*srcni) || !bool(*dstni)) {
        set_response(response, PMGDCmdResponse::Empty,
                        "Empty node iterators for adding edge");
        return -1;
    }

    long eid = 0;
    // TODO: Partition code goes here
    for ( ; *srcni; srcni->next()) {
        Node &src = **srcni;
        for ( ; *dstni; dstni->next()) {
            Node &dst = **dstni;
            Edge &e = _db->add_edge(src, dst, sid);
            for (int i = 0; i < ce.edge().properties_size(); ++i) {
                const PMGDProp &p = ce.edge().properties(i);
                set_property(e, p);
            }

            eid = _db->get_id(e);
        }
        dstni->reset();
    }
    srcni->reset();

    set_response(response, protobufs::EdgeID, PMGDCmdResponse::Success);
    // ID of the last edge added
    response->set_op_int_value(eid);
    return 0;
}

template <class Element>
void PMGDQueryHandler::set_property(Element &e, const PMGDProp &p)
{
    switch(p.type()) {
    case PMGDProp::BooleanType:
        e.set_property(p.key().c_str(), p.bool_value());
        break;
    case PMGDProp::IntegerType:
        e.set_property(p.key().c_str(), (long long)p.int_value());
        break;
    case PMGDProp::StringType:
        e.set_property(p.key().c_str(), p.string_value());
        break;
    case PMGDProp::FloatType:
        e.set_property(p.key().c_str(), p.float_value());
        break;
    case PMGDProp::TimeType:
    {
        struct tm tm_e;
        int hr, min;
        unsigned long usec;
        string_to_tm(p.time_value(), &tm_e, &usec, &hr, &min);
        Time t_e(&tm_e, usec, hr, min);  // time diff
        e.set_property(p.key().c_str(), t_e);
        break;
    }
    case PMGDProp::BlobType:
        e.set_property(p.key().c_str(), p.blob_value());
    }
}

int PMGDQueryHandler::query_node(const protobufs::QueryNode &qn,
                                    PMGDCmdResponse *response)
{
    ReusableNodeIterator *start_ni = NULL;
    PMGD::Direction dir;
    StringID edge_tag;

    if (qn.p_op() == protobufs::Or) {
        set_response(response, PMGDCmdResponse::Error,
                       "Or operation not implemented\n");
        return -1;
    }

    long id = qn.identifier();
    if (id >= 0 && _cached_nodes.find(id) != _cached_nodes.end()) {
        set_response(response, PMGDCmdResponse::Error,
                       "Reuse of _ref value\n");
        return -1;
    }

    bool has_link = qn.has_link();
    if (has_link)  { // case where link is used.
        const protobufs::LinkInfo &link = qn.link();
        if (link.nb_unique()) {  // TODO Add support for unique neighbors across iterators
            set_response(response, PMGDCmdResponse::Error,
                           "Non-repeated neighbors not supported\n");
            return -1;
        }
        long start_id = link.start_identifier();
        auto start = _cached_nodes.find(start_id);
        if (start == _cached_nodes.end()) {
            set_response(response, PMGDCmdResponse::Error,
                           "Undefined _ref value used in link\n");
            return -1;
        }
        start_ni = start->second;

        dir = (PMGD::Direction)link.dir();
        edge_tag = (link.edgetag_oneof_case() == protobufs::LinkInfo::kETagid)
                        ? StringID(link.e_tagid())
                        : StringID(link.e_tag().c_str());
    }

    StringID search_node_tag = (qn.tag_oneof_case() == PMGDQueryNode::kTagid)
                                ? StringID(qn.tagid())
                                : StringID(qn.tag().c_str());

    SearchExpression search(*_db, search_node_tag);

    for (int i = 0; i < qn.predicates_size(); ++i) {
        const PMGDPropPred &p_pp = qn.predicates(i);
        PropertyPredicate j_pp = construct_search_term(p_pp);
        search.add(j_pp);
    }

    NodeIterator ni = has_link ?
                       PMGD::NodeIterator(new MultiNeighborIteratorImpl(start_ni, search, dir, edge_tag))
                       : search.eval_nodes();
    if (!bool(ni)) {
        set_response(response, PMGDCmdResponse::Empty,
                       "Null search iterator\n");
        if (has_link)
            start_ni->reset();
        return -1;
    }

    // Set these in case there is no results block.
    set_response(response, qn.r_type(), PMGDCmdResponse::Success);

    // TODO: Also, this triggers a copy of the SearchExpression object
    // via the SearchExpressionIterator class, which might be slow,
    // especially with a lot of property constraints. Might need another
    // way for it.
    if (!(id >= 0 || qn.unique() || qn.sort())) {
        // If not reusable
        build_results<NodeIterator>(ni, qn, response);

        // Make sure the starting iterator is reset for later use.
        if (has_link)
            start_ni->reset();
        return 0;
    }

    ReusableNodeIterator *tni =  new ReusableNodeIterator(ni);

    if (qn.unique()) {
        tni->next();
        if (bool(*tni)) {  // Not unique and that is an error here.
            set_response(response, PMGDCmdResponse::NotUnique,
                           "Query response not unique\n");
            if (has_link)
                start_ni->reset();
            delete tni;
            return -1;
        }
        tni->reset();
    }

    if (qn.sort())
        tni->sort(qn.sort_key().c_str());

    if (qn.r_type() != protobufs::Cached)
        build_results<ReusableNodeIterator>(*tni, qn, response);

    if (id >= 0) {
        // We have to traverse the current iterator fully, so we can
        // reset start_ni.
        if (has_link)
            tni->traverse_all();
        tni->reset();
        _cached_nodes[id] = tni;
    }
    else
        delete tni;

    // If there is a link, we have to make sure the start_ni can be reset.
    if (has_link)
        start_ni->reset();

    return 0;
}

PropertyPredicate PMGDQueryHandler::construct_search_term(const PMGDPropPred &p_pp)
{
    StringID key = (p_pp.key_oneof_case() == 2) ? StringID(p_pp.keyid()) : StringID(p_pp.key().c_str());

    // Assumes exact match between enum values
    // TODO Maybe have some way of verifying certain such assumptions at start?
    PropertyPredicate::Op op = (PropertyPredicate::Op)p_pp.op();
    if (op == PropertyPredicate::DontCare)
        return PropertyPredicate(key);
    if (op < PropertyPredicate::GeLe)
        return PropertyPredicate(key, op, construct_search_property(p_pp.v1()));
    return PropertyPredicate(key, op, construct_search_property(p_pp.v1()),
                               construct_search_property(p_pp.v2()));
}

Property PMGDQueryHandler::construct_search_property(const PMGDProp &p)
{
    switch(p.type()) {
    case PMGDProp::BooleanType:
        return Property(p.bool_value());
    case PMGDProp::IntegerType:
        return Property((long long)p.int_value());
    case PMGDProp::StringType:
        return Property(p.string_value());
    case PMGDProp::FloatType:
        return Property(p.float_value());
    case PMGDProp::TimeType:
    {
        struct tm tm_e;
        int hr, min;
        unsigned long usec;
        string_to_tm(p.time_value(), &tm_e, &usec, &hr, &min);
        Time t_e(&tm_e, usec, hr, min);  // time diff
        return Property(t_e);
    }
    case PMGDProp::BlobType:
        // We throw here to avoid extra work when going through
        // multiple levels of calls.
        throw PMGDException(PropertyTypeInvalid, "Search on blob property not permitted");
    }
}

namespace VDMS {
    template
    void PMGDQueryHandler::build_results<PMGD::NodeIterator>(PMGD::NodeIterator &ni,
                                                  const protobufs::QueryNode &qn,
                                                  PMGDCmdResponse *response);
    template
    void PMGDQueryHandler::build_results<PMGDQueryHandler::ReusableNodeIterator>(
                                                  PMGDQueryHandler::ReusableNodeIterator &ni,
                                                  const protobufs::QueryNode &qn,
                                                  PMGDCmdResponse *response);
};

template <class Iterator>
void PMGDQueryHandler::build_results(Iterator &ni,
                                      const protobufs::QueryNode &qn,
                                      PMGDCmdResponse *response)
{
    bool avg = false;
    size_t limit = qn.limit() > 0 ? qn.limit() : std::numeric_limits<size_t>::max();
    size_t count = 0;
    switch(qn.r_type()) {
    case protobufs::List:
    {
        std::vector<StringID> keyids;
        for (int i = 0; i < qn.response_keys_size(); ++i)
            keyids.push_back(StringID(qn.response_keys(i).c_str()));

        auto& rmap = *(response->mutable_prop_values());
        for (; ni; ni.next()) {
            for (int i = 0; i < keyids.size(); ++i) {
                Property j_p;
                PMGDPropList &list = rmap[qn.response_keys(i)];
                PMGDProp *p_p = list.add_values();
                if (!ni->check_property(keyids[i], j_p)) {
                    construct_missing_property(p_p);
                    continue;
                }
                construct_protobuf_property(j_p, p_p);
            }
            count++;
            if (count >= limit)
                break;
        }
        response->set_op_int_value(count);
        break;
    }
    case protobufs::Count:
    {
        for (; ni; ni.next())
            count++;
        response->set_op_int_value(count);
        break;
    }
    // Next two assume that the property requested is either Int or Float.
    // Also, only looks at the first property specified.
    case protobufs::Average:
        avg = true;
    case protobufs::Sum:
    {
        // We currently only use the first property key even if multiple
        // are provided. And we can assume that the syntax checker makes
        // sure of getting one for sure.
        StringID keyid(qn.response_keys(0).c_str());
        if (ni->get_property(keyid).type() == PropertyType::Integer) {
            size_t sum = 0;
            for (; ni; ni.next()) {
                sum += ni->get_property(keyid).int_value();
                count++;
                if (count >= limit)
                    break;
            }
            if (avg)
                response->set_op_float_value((double)sum / count);
            else
                response->set_op_int_value(sum);
        }
        else if (ni->get_property(keyid).type() == PropertyType::Float) {
            double sum = 0.0;
            for (; ni; ni.next()) {
                sum += ni->get_property(keyid).float_value();
                count++;
                if (count >= limit)
                    break;
            }
            if (avg)
                response->set_op_float_value(sum / count);
            else
                response->set_op_float_value(sum);
        }
        else {
            set_response(response, PMGDCmdResponse::Error,
                           "Wrong first property for sum/average\n");
        }
        break;
    }
    case protobufs::NodeID:
    {
        // Makes sense only when unique was used. Otherwise it sets the
        // int value to the global id of the last node in the iterator.
        for (; ni; ni.next())
            response->set_op_int_value(ni->get_id());
        break;
    }
    default:
        set_response(response, PMGDCmdResponse::Error,
                       "Unknown operation type for query\n");
    }
}

void PMGDQueryHandler::construct_protobuf_property(const Property &j_p, PMGDProp *p_p)
{
    // Assumes matching enum values!
    p_p->set_type((PMGDProp::PropertyType)j_p.type());
    switch(j_p.type()) {
    case PropertyType::Boolean:
        p_p->set_bool_value(j_p.bool_value());
        break;
    case PropertyType::Integer:
        p_p->set_int_value(j_p.int_value());
        break;
    case PropertyType::String:
        p_p->set_string_value(j_p.string_value());
        break;
    case PropertyType::Float:
        p_p->set_float_value(j_p.float_value());
        break;
    case PropertyType::Time:
        p_p->set_time_value(time_to_string(j_p.time_value()));
        break;
    case PropertyType::Blob:
        p_p->set_blob_value(j_p.blob_value().value, j_p.blob_value().size);
    }
}

void PMGDQueryHandler::construct_missing_property(PMGDProp *p_p)
{
    // Assumes matching enum values!
    p_p->set_type(PMGDProp::StringType);
    p_p->set_string_value("Missing property");
}
