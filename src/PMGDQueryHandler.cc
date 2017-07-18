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

std::vector<protobufs::CommandResponse *> PMGDQueryHandler::process_queries(std::vector<protobufs::Command *> cmds)
{
    std::vector<protobufs::CommandResponse *> responses;
    for (auto cmd : cmds) {
        protobufs::CommandResponse *response = new protobufs::CommandResponse();
        process_query(cmd, response);
        responses.push_back(response);
    }
    // TODO Assuming a move rather than vector copy here.
    // Will the response pointers be deleted when the vector goes out of scope?
    return responses;
}

void PMGDQueryHandler::process_query(protobufs::Command *cmd,
                                     protobufs::CommandResponse *response)
{
    try {
        // TODO: Need a shutdown command for the thread
        int code = cmd->cmd_id();
        switch (code) {
            case protobufs::Command::TxBegin:
            {
                int txID = cmd->tx_id();
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
                mNodes.clear();
                mEdges.clear();
                delete _tx;
                _tx = NULL;
                response->set_error_code(protobufs::CommandResponse::Abort);
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
        response->set_error_code(protobufs::CommandResponse::Exception);
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
        throw JarvisException(NotImplemented);
    }
}

void PMGDQueryHandler::add_node(const protobufs::AddNode &cn,
                                  protobufs::CommandResponse *response)
{
    // Presumably this node gets placed here.
    StringID sid(cn.node().tag().c_str());
    Node &n = _db->add_node(sid);
    mNodes.insert(std::pair<int, Node *>(cn.identifier(), &n));

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
    // Assumes these entries are found.
    Node &src = *(mNodes.at(ce.edge().src()));
    Node &dst = *(mNodes.at(ce.edge().dst()));
    Edge &e = _db->add_edge(src, dst, sid);
    mEdges.insert(std::pair<int, Edge *>(ce.identifier(), &e));

    for (int i = 0; i < ce.edge().properties_size(); ++i) {
        const protobufs::Property &p = ce.edge().properties(i);
        set_property(e, p);
    }

    response->set_error_code(protobufs::CommandResponse::Success);
    response->set_r_type(protobufs::EdgeID);
    // TODO: Partition code goes here
    // For now, fill in the single system edge id
    response->set_op_int_value(_db->get_id(e));
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
        throw JarvisException(PropertyTypeInvalid);
    }
}

PropertyPredicate PMGDQueryHandler::construct_search_term(const protobufs::PropertyPredicate &p_pp)
{
    StringID *key;

    if (p_pp.key_oneof_case() == 2) {
        // TODO For now, assuming that this is the ID in JL String Table.
        // I think this should be a client specified identifier which is maintained
        // in a map of (client string id, StringID &)
        StringID tmp(p_pp.keyid());
        key = &tmp;
    }
    else {
        StringID tmp(p_pp.key().c_str());
        key = &tmp;   // Points into PMGD anyway. For NOW
    }

    // Assumes exact match between enum values
    // TODO Maybe have some way of verifying certain such assumptions at start?
    PropertyPredicate::Op op = (PropertyPredicate::Op)p_pp.op();
    if (op == PropertyPredicate::DontCare)
        return PropertyPredicate(*key);
    if (op < PropertyPredicate::GeLe)
        return PropertyPredicate(*key, op, construct_search_property(p_pp.v1()));
    return PropertyPredicate(*key, op, construct_search_property(p_pp.v1()),
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
        throw JarvisException(PropertyTypeInvalid);
    }
}

void PMGDQueryHandler::query_node(const protobufs::QueryNode &qn,
                                    protobufs::CommandResponse *response)
{
    StringID *search_node_tag;

    if (qn.tag_oneof_case() == 2) {
        // TODO For now, assuming that this is the ID in JL String Table.
        // I think this should be a client specified identifier which is maintained
        // in a map of (client string id, StringID &)
        StringID tmp(qn.tagid());
        search_node_tag = &tmp;
    }
    else {
        StringID tmp(qn.tag().c_str());
        search_node_tag = &tmp;   // Points into PMGD anyway. For NOW
    }
        
    SearchExpression search(*_db, *search_node_tag);

    if (qn.p_op() == protobufs::Or)
        throw JarvisException(NotImplemented);

    for (int i = 0; i < qn.predicates_size(); ++i) {
        const protobufs::PropertyPredicate &p_pp = qn.predicates(i);
        PropertyPredicate j_pp = construct_search_term(p_pp); 
        search.Add(j_pp); 
    }

    NodeIterator ni = search.EvalNodes();

    // TODO This should really be translated at some global level. Either
    // this class or maybe even the request server main handler.
    std::vector<StringID> keyids;
    for (int i = 0; i < qn.response_keys_size(); ++i)
        keyids.push_back(StringID(qn.response_keys(i).c_str()));
    printf("Number of requested keys: %ld\n", keyids.size());

    if (ni) {
        if (qn.r_type() == protobufs::List) {
            auto& rmap = *(response->mutable_prop_values());
            for (; ni; ni.next()) {
                for (int i = 0; i < keyids.size(); ++i) {
                    protobufs::PropertyList &list = rmap[qn.response_keys(i)];
                    protobufs::Property *p_p = list.add_values();
                    Property j_p = ni->get_property(keyids[i]);
                    // Assumes matching enum values!
                    p_p->set_type((protobufs::Property::PropertyType)j_p.type());
                    construct_protobuf_property(j_p, p_p);
                }
            }
        }
    }
    else
        printf("No matching nodes\n");

    response->set_r_type(qn.r_type());
    response->set_error_code(protobufs::CommandResponse::Success);
}
