#include "PMGDQueryHandler.h"
#include "util.h"   // Jarvis util

// TODO In the complete version of Athena, this file will live
// within PMGD which would replace the Jarvis namespace. Some of
// these code pieces are temporary.
using namespace pmgd;
using namespace Jarvis;
using namespace athena;

PMGDQueryHandler::PMGDQueryHandler(Graph *db, std::mutex *mtx)
{
    _db = db;
    _dblock = mtx;
    _tx = NULL;
}

std::vector<std::vector<protobufs::CommandResponse *>>
              PMGDQueryHandler::process_queries(std::vector<protobufs::Command *> cmds,
              int num_queries)
{
    int curr_query = 0;
    std::vector<std::vector<protobufs::CommandResponse *>> responses(num_queries);

    for (auto cmd : cmds) {
        protobufs::CommandResponse *response = new protobufs::CommandResponse();
        std::vector<protobufs::CommandResponse *> &resp_v = responses[cmd->cmd_grp_id()];
        try {
            process_query(cmd, response);
        }
        catch (Exception e) {
            for (unsigned i = 0; i < curr_query; ++i) {
                unsigned size = responses[i].size();
                for (unsigned j = 0; j < size; ++j) {
                    if (responses[i][j] != NULL)
                        delete responses[i][j];
                    responses[i].clear();
                }
            }
            responses.resize(1);

            // Since we have shortened the container, this reference will still remain
            // valid. So we can reuse the 0th spot.
            response->set_cmd_grp_id(0);
            std::vector<protobufs::CommandResponse *> &resp_v1 = responses[0];
            resp_v1.push_back(response);
            return responses;
        }
        resp_v.push_back(response);
        curr_query++;
    }
    // TODO Assuming a move rather than vector copy here.
    // Will the response pointers be deleted when the vector goes out of scope?
    return responses;
}

void PMGDQueryHandler::process_query(protobufs::Command *cmd,
                                     protobufs::CommandResponse *response)
{
    try {
        int code = cmd->cmd_id();
        response->set_cmd_grp_id(cmd->cmd_grp_id());
        switch (code) {
            case protobufs::Command::TxBegin:
            {
                _dblock->lock();

                // TODO: Needs to distinguish transaction parameters like RO/RW
                _tx = new Transaction(*_db, Transaction::ReadWrite);
                response->set_error_code(protobufs::CommandResponse::Success);
                break;
            }
            case protobufs::Command::TxCommit:
            {
                _tx->commit();
                _dblock->unlock();
                // TODO Clearing might not be needed if it goes out of scope anyway
                // TODO Need to free pointers at some point
                mNodes.clear();
                mEdges.clear();
                delete _tx;
                _tx = NULL;
                response->set_error_code(protobufs::CommandResponse::Success);
                break;
            }
            case protobufs::Command::TxAbort:
            {
                _dblock->unlock();
                // TODO Clearing might not be needed if it goes out of scope anyway
                // TODO Need to free pointers at some point
                mNodes.clear();
                mEdges.clear();
                delete _tx;
                _tx = NULL;
                response->set_error_code(protobufs::CommandResponse::Abort);
                response->set_error_msg("Abort called");
                break;
            }
            case protobufs::Command::AddNode:
                add_node(cmd->add_node(), response);
                break;
            case protobufs::Command::AddEdge:
                add_edge(cmd->add_edge(), response);
                break;
                //     case Command::CreateIndex:
                //         CreateIndex();
                //         break;
                //     case Command::SetProperty:
                //         SetProperty();
                //         break;
            case protobufs::Command::QueryNode:
                query_node(cmd->query_node(), response);
                break;
                //     case Command::RemoveNode:
                //         RemoveNode();
                //         break;
                //     case Command::RemoveEdge:
                //         RemoveEdge();
                //         break;
                //     case Command::RemoveNodeEdge:
                //         RemoveNodeEdge();
                //         break;
                //     case Command::Shutdown:
                //         mTX->commit();
                //         _dblock->unlock();
                //         delete c;
                //         return;

        }
    }
    catch (Exception e) {
        _dblock->unlock();
        mNodes.clear();
        mEdges.clear();
        delete _tx;
        _tx = NULL;
        response->set_error_code(protobufs::CommandResponse::Exception);
        response->set_error_msg(e.name + std::string(": ") +  e.msg);
        throw e;
    }
}

template <class Element>
void PMGDQueryHandler::set_property(Element &e, const protobufs::Property &p)
{
    switch(p.type()) {
    case protobufs::Property::BooleanType:
        e.set_property(p.key().c_str(), p.bool_value());
        break;
    case protobufs::Property::IntegerType:
        e.set_property(p.key().c_str(), (long long)p.int_value());
        break;
    case protobufs::Property::StringType:
        e.set_property(p.key().c_str(), p.string_value());
        break;
    case protobufs::Property::FloatType:
        e.set_property(p.key().c_str(), p.float_value());
        break;
    case protobufs::Property::TimeType:
    {
        struct tm tm_e;
        int hr, min;
        string_to_tm(p.time_value(), &tm_e, &hr, &min);
        Time t_e(&tm_e, hr, min);  // time diff
        e.set_property(p.key().c_str(), t_e);
        break;
    }
    case protobufs::Property::BlobType:
        throw JarvisException(NotImplemented, "Blob type not implemented");
    }
}

void PMGDQueryHandler::add_node(const protobufs::AddNode &cn,
                                  protobufs::CommandResponse *response)
{
    if (cn.has_query_node()) {
        const protobufs::QueryNode &qn = cn.query_node();
        // Have to query and check if this node is present.
        // TODO For now, assuming that this is the ID in JL String Table.
        // I think this should be a client specified identifier which is maintained
        // in a map of (client string id, StringID &)
        StringID search_node_tag = (qn.tag_oneof_case() == 4) ? StringID(qn.tagid())
                                    : StringID(qn.tag().c_str());

        // TODO Make a function or something out of this sequence.
        SearchExpression search(*_db, search_node_tag);

        if (qn.p_op() == protobufs::Or)
            throw JarvisException(NotImplemented, "Or operator not implemented");

        for (int i = 0; i < qn.predicates_size(); ++i) {
            const protobufs::PropertyPredicate &p_pp = qn.predicates(i);
            PropertyPredicate j_pp = construct_search_term(p_pp);
            search.add(j_pp);
        }

        NodeIterator ni = search.eval_nodes();
        if (bool(ni)) {
            // TODO This needs a reusable iterator
            Node &n = *ni;
            ni.next();
            if (bool(ni)) {  // Not unique and that is an error here.
                response->set_error_code(protobufs::CommandResponse::NotUnique);
                response->set_error_msg("No unique node found for the add node\n");
                return;
            }
            if (cn.identifier() >= 0)
                mNodes[cn.identifier()] = new NodeIterator(new NewNodeIteratorImpl(n));
            response->set_error_code(protobufs::CommandResponse::Exists);
            response->set_r_type(protobufs::NodeID);
            // TODO: Partition code goes here
            // For now, fill in the single system node id
            response->set_op_int_value(_db->get_id(n));
            return;
        }
    }

    // Since the node wasn't found, now add it.
    StringID sid(cn.node().tag().c_str());
    Node &n = _db->add_node(sid);
    if (cn.identifier() >= 0) {
        // Need a pointer type for NodeIterator due to its semantics.
        mNodes[cn.identifier()] = new NodeIterator(new NewNodeIteratorImpl(n));
    }

    for (int i = 0; i < cn.node().properties_size(); ++i) {
        const protobufs::Property &p = cn.node().properties(i);
        set_property(n, p);
    }

    response->set_error_code(protobufs::CommandResponse::Success);
    response->set_r_type(protobufs::NodeID);
    // TODO: Partition code goes here
    // For now, fill in the single system node id
    response->set_op_int_value(_db->get_id(n));
}

void PMGDQueryHandler::add_edge(const protobufs::AddEdge &ce,
                                  protobufs::CommandResponse *response)
{
    // Presumably this node gets placed here.
    StringID sid(ce.edge().tag().c_str());

    // Assumes there could be multiple.
    NodeIterator *srcni;
    NodeIterator *dstni;

    // Since _ref is optional, need to make sure the map has the
    // right reference.
    auto srcit = mNodes.find(ce.edge().src());
    auto dstit = mNodes.find(ce.edge().dst());
    if (srcit != mNodes.end() && dstit != mNodes.end()) {
        srcni = srcit->second;
        dstni = dstit->second;
    }
    else {
        response->set_error_code(protobufs::CommandResponse::Error);
        response->set_error_msg("Source/destination node references not found");
        return;
    }
    if (!srcni || !dstni) {
        response->set_error_code(protobufs::CommandResponse::Empty);
        response->set_error_msg("Empty node iterators for adding edge");
        return;
    }
    long eid = 0;
    // TODO: Partition code goes here
    for ( ; *srcni; srcni->next()) {
        Node &src = **srcni;
        for ( ; *dstni; dstni->next()) {
            Node &dst = **dstni;
            Edge &e = _db->add_edge(src, dst, sid);
            for (int i = 0; i < ce.edge().properties_size(); ++i) {
                const protobufs::Property &p = ce.edge().properties(i);
                set_property(e, p);
            }

            mEdges.insert(std::pair<int, Edge *>(ce.identifier(), &e));
            eid = _db->get_id(e);
        }
    }

    response->set_error_code(protobufs::CommandResponse::Success);
    response->set_r_type(protobufs::EdgeID);
    // ID of the last edge added
    response->set_op_int_value(eid);
}

// TODO: Do we need a separate function here or should we try to combine
// this with the set_property(). Avoids extra property copies?
Property PMGDQueryHandler::construct_search_property(const protobufs::Property &p)
{
    switch(p.type()) {
    case protobufs::Property::BooleanType:
        return Property(p.bool_value());
    case protobufs::Property::IntegerType:
        return Property((long long)p.int_value());
    case protobufs::Property::StringType:
        return Property(p.string_value());
    case protobufs::Property::FloatType:
        return Property(p.float_value());
    case protobufs::Property::TimeType:
    {
        struct tm tm_e;
        int hr, min;
        string_to_tm(p.time_value(), &tm_e, &hr, &min);
        Time t_e(&tm_e, hr, min);  // time diff
        return Property(t_e);
    }
    case protobufs::Property::BlobType:
        throw JarvisException(PropertyTypeInvalid, "Search on blob property not permitted");
    }
}

PropertyPredicate PMGDQueryHandler::construct_search_term(const protobufs::PropertyPredicate &p_pp)
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

void PMGDQueryHandler::construct_protobuf_property(const Property &j_p, protobufs::Property *p_p)
{
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
        throw JarvisException(PropertyTypeInvalid, "Blob property not supported");
    }
}

void PMGDQueryHandler::query_node(const protobufs::QueryNode &qn,
                                    protobufs::CommandResponse *response)
{
    NodeIterator *start_ni = NULL;
    Jarvis::Direction dir;
    StringID edge_tag;

    if (qn.p_op() == protobufs::Or)
        throw JarvisException(NotImplemented, "Or operation not implemented");

    bool has_link = qn.has_link();
    if (has_link)  { // case where link is used.
        const protobufs::LinkInfo &link = qn.link();
        if (link.nb_unique()) {  // TODO Add support for unique neighbors across iterators
            response->set_error_code(protobufs::CommandResponse::Error);
            response->set_error_msg("Non-repeated neighbors not supported\n");
            return;
        }

        dir = (Jarvis::Direction)link.dir();
        edge_tag = (link.e_tagid() == 4) ? StringID(link.e_tagid())
                        : StringID(link.e_tag().c_str());
        start_ni = mNodes[link.start_identifier()];
        // TODO Just until iterators can be reused, remove this iterator
        // from the map since it can only be used once and we can assume it
        // is being accessed here to be used.
        //mNodes.erase(qnb.start_identifier());
    }

    // TODO For now, assuming that this is the ID in JL String Table.
    // I think this should be a client specified identifier which is maintained
    // in a map of (client string id, StringID &)
    StringID search_node_tag = (qn.tag_oneof_case() == 4) ? StringID(qn.tagid())
                                : StringID(qn.tag().c_str());

    SearchExpression search(*_db, search_node_tag);

    for (int i = 0; i < qn.predicates_size(); ++i) {
        const protobufs::PropertyPredicate &p_pp = qn.predicates(i);
        PropertyPredicate j_pp = construct_search_term(p_pp);
        search.add(j_pp);
    }

    NodeIterator ni = has_link ?
                       Jarvis::NodeIterator(new MultiNeighborIteratorImpl(start_ni, search, dir, edge_tag))
                       : search.eval_nodes();
    if (!bool(ni)) {
        response->set_error_code(protobufs::CommandResponse::Empty);
        response->set_error_msg("Null search iterator\n");
        return;
    }

    // TODO This needs a reusable iterator and this check won't happen for Neighbor
    // case since unique = true is not supported.
    Node &n = *ni;  // outside if so it can be used for caching if needed.
    if (qn.unique()) {
        // TODO Really expensive without the reusable iterator
        NodeIterator tni = search.eval_nodes();
        tni.next();
        if (bool(tni)) {  // Not unique and that is an error here.
            response->set_error_code(protobufs::CommandResponse::NotUnique);
            response->set_error_msg("Query response not unique\n");
            return;
        }
    }
    if (qn.identifier() >= 0) {
        // TODO: If ni is now used, it will show up empty since it has been
        // moved to mNodes. Needs fixing for reuse.
        // TODO: Also, this triggers a copy of the SearchExpression object
        // via the SearchExpressionIterator class, which might be slow,
        // especially with a lot of property constraints. Might need another
        // way for it.
        mNodes[qn.identifier()] = qn.unique() ? new NodeIterator(new NewNodeIteratorImpl(n))
                                    : new NodeIterator(ni);
        response->set_r_type(protobufs::Cached);
        response->set_error_code(protobufs::CommandResponse::Success);
        return;
    }

    // TODO This should really be translated at some global level. Either
    // this class or maybe even the request server main handler.
    std::vector<StringID> keyids;
    for (int i = 0; i < qn.response_keys_size(); ++i)
        keyids.push_back(StringID(qn.response_keys(i).c_str()));

    response->set_r_type(qn.r_type());
    response->set_error_code(protobufs::CommandResponse::Success);

    bool avg = false;
    size_t count = 0;
    switch(qn.r_type()) {
    case protobufs::List:
    {
        auto& rmap = *(response->mutable_prop_values());
        for (; ni; ni.next()) {
            count++;
            for (int i = 0; i < keyids.size(); ++i) {
                protobufs::PropertyList &list = rmap[qn.response_keys(i)];
                Property j_p;
                if (!ni->check_property(keyids[i], j_p))
                    continue;
                // Assumes matching enum values!
                protobufs::Property *p_p = list.add_values();
                p_p->set_type((protobufs::Property::PropertyType)j_p.type());
                construct_protobuf_property(j_p, p_p);
            }
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
        if (ni->get_property(keyids[0]).type() == PropertyType::Integer) {
            size_t sum = 0;
            for (; ni; ni.next()) {
                sum += ni->get_property(keyids[0]).int_value();
                count++;
            }
            if (avg)
                response->set_op_float_value((double)sum / count);
            else
                response->set_op_int_value(sum);
        }
        else if (ni->get_property(keyids[0]).type() == PropertyType::Float) {
            double sum = 0.0;
            for (; ni; ni.next()) {
                sum += ni->get_property(keyids[0]).float_value();
                count++;
            }
            if (avg)
                response->set_op_float_value(sum / count);
            else
                response->set_op_float_value(sum);
        }
        else {
            response->set_error_code(protobufs::CommandResponse::Error);
            response->set_error_msg("Wrong first property for sum/average\n");
        }
        break;
    }
    default:
        response->set_error_code(protobufs::CommandResponse::Error);
        response->set_error_msg("Unknown operation type for query\n");
    }
}
