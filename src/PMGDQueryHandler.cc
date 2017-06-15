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

void PMGDQueryHandler::process_queries(std::vector<protobufs::Command *> cmds)
{
    for (auto cmd : cmds)
        process_query(cmd);
}

void PMGDQueryHandler::process_query(protobufs::Command *cmd)
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
                break;
            }
            case protobufs::Command::TxEnd:
            {
                _tx->commit();
                _dblock->unlock();
                mNodes.clear();
                mEdges.clear();
                delete _tx;
                _tx = NULL;
                break;
            }
            case protobufs::Command::AddNode:
                add_node(cmd->add_node());
                break;
                //     case Command::CreateEdge:
                //         CreateEdge();
                //         break;
                //     case Command::CreateIndex:
                //         CreateIndex();
                //         break;
                //     case Command::SetProperty:
                //         SetProperty();
                //         break;
                //     case Command::Query:
                //         Query();
                //         break;
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
    }
}

protobufs::CommandResponse PMGDQueryHandler::add_node(const protobufs::AddNode &cn)
{
    // TODO: Partition code goes here
    long long global_id = 0;

    // Presumably this node gets placed here.
    StringID sid(cn.node().tag().c_str());
    Node &n = _db->add_node(sid);
    mNodes.insert(std::pair<int, Node *>(cn.identifier(), &n));

    for (int i = 0; i < cn.node().properties_size(); ++i) {
        const protobufs::Property &p = cn.node().properties(i);
        switch(p.type()) {
        case protobufs::Property::BooleanType:
            n.set_property(p.key().c_str(), p.bool_value());
            break;
        case protobufs::Property::IntegerType:
            n.set_property(p.key().c_str(), (long long)p.int_value());
            break;
        case protobufs::Property::StringType:
            n.set_property(p.key().c_str(), p.string_value());
            break;
        case protobufs::Property::FloatType:
            n.set_property(p.key().c_str(), p.float_value());
            break;
        case protobufs::Property::TimeType:
            struct tm tm_e;
            int hr, min;
            string_to_tm(p.time_value(), &tm_e, &hr, &min);
            Time t_e(&tm_e, hr, min);  // time diff
            n.set_property(p.key().c_str(), t_e);
            break;
        }
    }

    protobufs::CommandResponse response;
    response.set_error_code(protobufs::CommandResponse::Success);

    return response;
}
