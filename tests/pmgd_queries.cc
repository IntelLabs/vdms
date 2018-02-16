#include <mutex>
#include <vector>

#include "protobuf/pmgdMessages.pb.h" // Protobuff implementation
#include "jarvis.h"
#include "PMGDQueryHandler.h"

using namespace pmgd;
// TODO Need the following namespace just for now.
using namespace Jarvis;
using namespace athena;
using namespace std;

int main(int argc, char **argv)
{
    printf("Testing PMGD protobuf handler\n");

    Graph db("qhgraph", Graph::Create);

    // Since PMGD is still single threaded, provide a lock for the DB
    mutex dblock;
    PMGDQueryHandler qh(&db, &dblock);

    vector<protobufs::Command *> cmds;

    printf("Simple add test\n");
    {
        int txid = 1;
        protobufs::Command cmdtx;
        cmdtx.set_cmd_id(protobufs::Command::TxBegin);
        cmdtx.set_tx_id(txid);
        cmds.push_back(&cmdtx);

        protobufs::Command cmdadd;
        cmdadd.set_cmd_id(protobufs::Command::AddNode);
        cmdadd.set_tx_id(txid);
        protobufs::AddNode *an = cmdadd.mutable_add_node();
        an->set_identifier(1);
        protobufs::Node *n = an->mutable_node();
        n->set_tag("Patient");
        protobufs::Property *p = n->add_properties();
        p->set_type(protobufs::Property::StringType);
        p->set_key("Email");
        p->set_string_value("a.b@z.com");
        p = n->add_properties();
        p->set_type(protobufs::Property::IntegerType);
        p->set_key("Age");
        p->set_int_value(86);
        p = n->add_properties();
        p->set_type(protobufs::Property::TimeType);
        p->set_key("Birthday");
        p->set_time_value("Sat Nov 1 18:59:24 PDT 1930");
        cmds.push_back(&cmdadd);

        protobufs::Command cmdadd1;
        cmdadd1.set_cmd_id(protobufs::Command::AddNode);
        cmdadd1.set_tx_id(txid);
        protobufs::AddNode *an1 = cmdadd1.mutable_add_node();
        an1->set_identifier(2);
        protobufs::Node *n1 = an1->mutable_node();
        n1->set_tag("Patient");
        p = n1->add_properties();
        p->set_type(protobufs::Property::StringType);
        p->set_key("Email");
        p->set_string_value("b.c@z.com");
        p = n1->add_properties();
        p->set_type(protobufs::Property::IntegerType);
        p->set_key("Age");
        p->set_int_value(76);
        p = n1->add_properties();
        p->set_type(protobufs::Property::TimeType);
        p->set_key("Birthday");
        p->set_time_value("Sun Dec 1 18:59:24 PDT 1940");
        cmds.push_back(&cmdadd1);

        protobufs::Command cmdtxend;
        cmdtxend.set_cmd_id(protobufs::Command::TxEnd);
        cmdtxend.set_tx_id(txid);
        cmds.push_back(&cmdtxend);

        qh.process_queries(cmds);
    }

    return 0;
}
