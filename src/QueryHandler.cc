#include "QueryHandler.h"

using namespace athena;
using namespace Jarvis;

QueryHandler::QueryHandler(Graph *db, std::mutex *mtx)
{
    _db = db;
    _dblock = mtx;
}

void QueryHandler::process_query(comm::Connection *c)
{
    CommandHandler handler(c);

    try {
        protobufs::queryMessage cmd = handler.get_command();
        // TODO: Need a shutdown command for the thread

        // int code = cmd.cmd_id();
        // assert(code == protobufs::Command::TxBegin);
        // _dblock->lock();
        // int txID = cmd.tx_id();
        // // TODO: Needs to distinguish transaction parameters like RO/RW
        // Jarvis::Transaction * _tx = new Transaction(*_db, Transaction::ReadWrite);
        // while (true) { // Loop until we get end-tx command
        //     switch (code) {
        //     case protobufs::Command::TxEnd:
        //     {
        //         _tx->commit();
        //         _dblock->unlock();
        //         mNodes.clear();
        //         mEdges.clear();
        //         goto NextTx;
        //     }
        //     case protobufs::Command::CreateNode:
        //         create_node(handler, cmd.create_node());
        //         break;

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

        //     }
    //     }
    // NextTx:;
    }
    catch (Exception e) {
        _dblock->unlock();
        delete c;
    }
}

// void QueryHandler::create_node(CommandHandler &handler, const protobufs::CreateNode &cn)
// {
//     // TODO: Partition code goes here
//     long long global_id = 0;
//     // Presumably this node gets placed here.
//     StringID sid(cn.node().tag().c_str());
//     Node &n = _db->add_node(sid);
//     mNodes.insert(std::pair<int, Node *>(cn.identifier(), &n));

//     protobufs::CommandResponse response;
//     response.set_error_code(protobufs::CommandResponse::Success);
//     handler.send_response(response);
// }
