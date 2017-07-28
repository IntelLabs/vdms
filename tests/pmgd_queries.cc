#include "gtest/gtest.h"

#include <mutex>
#include <vector>

#include "protobuf/pmgdMessages.pb.h" // Protobuff implementation
#include "jarvis.h"
#include "PMGDQueryHandler.h"

#include <stdlib.h>     /* system, NULL, EXIT_FAILURE */

using namespace pmgd;
// TODO Need the following namespace just for now.
using namespace Jarvis;
using namespace athena;
using namespace std;

#define MALE 0
#define FEMALE 1

void add_patient(protobufs::Command &cmdadd, int id, string name, int age,
                  string dob, string email, int sex)
{
    cmdadd.set_cmd_id(protobufs::Command::AddNode);
    protobufs::AddNode *an = cmdadd.mutable_add_node();
    an->set_identifier(id);
    protobufs::Node *n = an->mutable_node();
    n->set_tag("Patient");
    protobufs::Property *p = n->add_properties();
    p->set_type(protobufs::Property::StringType);
    p->set_key("Name");
    p->set_string_value(name);
    p = n->add_properties();
    p->set_type(protobufs::Property::IntegerType);
    p->set_key("Age");
    p->set_int_value(age);
    p = n->add_properties();
    p->set_type(protobufs::Property::TimeType);
    p->set_key("Birthday");
    p->set_time_value(dob);
    p = n->add_properties();
    p->set_type(protobufs::Property::StringType);
    p->set_key("Email");
    p->set_string_value(email);
    p = n->add_properties();
    p->set_type(protobufs::Property::IntegerType);
    p->set_key("Sex");
    p->set_int_value(sex);
}

TEST(PMGDQueryHandler, addTest)
{
    printf("Testing PMGD add protobuf handler\n");

    int ret = system("rm -r qhgraph");
    Graph db("qhgraph", Graph::Create);

    // Since PMGD is still single threaded, provide a lock for the DB
    mutex dblock;
    PMGDQueryHandler qh(&db, &dblock);

    vector<protobufs::Command *> cmds;

    {
        int txid = 1, patientid = 1, eid = 1;
        protobufs::Command cmdtx;
        cmdtx.set_cmd_id(protobufs::Command::TxBegin);
        cmdtx.set_tx_id(txid);
        cmds.push_back(&cmdtx);

        protobufs::Command cmdadd;
        cmdadd.set_tx_id(txid);
        add_patient(cmdadd, patientid++, "John Doe", 86, "Sat Nov 1 18:59:24 PDT 1930",
                  "john.doe@abc.com", MALE);
        cmds.push_back(&cmdadd);

        protobufs::Command cmdadd1;
        cmdadd1.set_tx_id(txid);
        add_patient(cmdadd1, patientid++, "Jane Doe", 80, "Sat Oct 1 17:59:24 PDT 1936",
                  "jane.doe@abc.com", FEMALE);
        cmds.push_back(&cmdadd1);

        protobufs::Command cmdedge1;
        cmdedge1.set_tx_id(txid);
        cmdedge1.set_cmd_id(protobufs::Command::AddEdge);
        protobufs::AddEdge *ae = cmdedge1.mutable_add_edge();
        ae->set_identifier(eid++);
        protobufs::Edge *e = ae->mutable_edge();
        e->set_src(1);
        e->set_dst(2);
        e->set_tag("Married");
        protobufs::Property *p = e->add_properties();
        p->set_type(protobufs::Property::TimeType);
        p->set_key("Since");
        p->set_time_value("Sat Sep 1 19:59:24 PDT 1956");
        cmds.push_back(&cmdedge1);

        protobufs::Command cmdadd2;
        cmdadd2.set_tx_id(txid);
        add_patient(cmdadd2, patientid++, "Alice Crypto", 70, "Sat Nov 1 17:59:24 PDT 1946",
                  "alice.crypto@xyz.com", FEMALE);
        cmds.push_back(&cmdadd2);

        protobufs::Command cmdadd3;
        cmdadd3.set_tx_id(txid);
        add_patient(cmdadd3, patientid++, "Bob Crypto", 70, "Sat Nov 30 7:59:24 PDT 1946",
                  "bob.crypto@xyz.com", MALE);
        cmds.push_back(&cmdadd3);

        protobufs::Command cmdedge2;
        cmdedge2.set_tx_id(txid);
        cmdedge2.set_cmd_id(protobufs::Command::AddEdge);
        ae = cmdedge2.mutable_add_edge();
        ae->set_identifier(eid++);
        e = ae->mutable_edge();
        e->set_src(3);
        e->set_dst(4);
        e->set_tag("Married");
        p = e->add_properties();
        p->set_type(protobufs::Property::TimeType);
        p->set_key("Since");
        p->set_time_value("Wed Dec 2 19:59:24 PDT 1970");
        cmds.push_back(&cmdedge2);

        protobufs::Command cmdtxcommit;
        cmdtxcommit.set_cmd_id(protobufs::Command::TxCommit);
        cmdtxcommit.set_tx_id(txid);
        cmds.push_back(&cmdtxcommit);

        vector<protobufs::CommandResponse *> responses = qh.process_queries(cmds);
        int nodeids = 1, edgeids = 1;
        for (auto it : responses) {
            ASSERT_EQ(it->error_code(), protobufs::CommandResponse::Success) << "Unsuccessful TX";
            if (it->r_type() == protobufs::NodeID) {
                long nodeid = it->op_int_value();
                EXPECT_EQ(nodeid, nodeids++) << "Unexpected node id";
            }
            else if (it->r_type() == protobufs::EdgeID) {
                long edgeid = it->op_int_value();
                EXPECT_EQ(edgeid, edgeids++) << "Unexpected edge id";
            }
        }
    }
}

void print_property(const string &key, const protobufs::Property &p)
{
    switch(p.type()) {
    case protobufs::Property::BooleanType:
        printf("key: %s, value: %d\n", key.c_str(), p.bool_value());
        break;
    case protobufs::Property::IntegerType:
        printf("key: %s, value: %ld\n", key.c_str(), p.int_value());
        break;
    case protobufs::Property::StringType:
    case protobufs::Property::TimeType:
        printf("key: %s, value: %s\n", key.c_str(), p.string_value().c_str());
        break;
    case protobufs::Property::FloatType:
        printf("key: %s, value: %lf\n", key.c_str(), p.float_value());
        break;
    default:
        printf("Unknown\n");
    }
}

TEST(PMGDQueryHandler, queryTest)
{
    printf("Testing PMGD query protobuf handler\n");

    Graph db("qhgraph");

    // Since PMGD is still single threaded, provide a lock for the DB
    mutex dblock;
    PMGDQueryHandler qh(&db, &dblock);

    vector<protobufs::Command *> cmds;

    {
        int txid = 1;
        protobufs::Command cmdtx;
        cmdtx.set_cmd_id(protobufs::Command::TxBegin);
        cmdtx.set_tx_id(txid);
        cmds.push_back(&cmdtx);

        protobufs::Command cmdquery;
        cmdquery.set_cmd_id(protobufs::Command::QueryNode);
        cmdquery.set_tx_id(txid);
        protobufs::QueryNode *qn = cmdquery.mutable_query_node();
        qn->set_identifier(1);
        qn->set_tag("Patient");
        qn->set_p_op(protobufs::QueryNode::And);
        protobufs::PropertyPredicate *pp = qn->add_predicates();
        pp->set_key("Email");
        pp->set_op(protobufs::PropertyPredicate::Gt);
        protobufs::Property *p = pp->mutable_v1();
        p->set_type(protobufs::Property::StringType);
        // I think the key is not required here.
        p->set_key("Email");
        p->set_string_value("j");
        qn->set_r_type(protobufs::List);
        string *key = qn->add_response_keys();
        *key = "Email";
        key = qn->add_response_keys();
        *key = "Age";
        cmds.push_back(&cmdquery);

        // No need to commit in this case. So just end TX
        protobufs::Command cmdtxend;
        // Commit here doesn't change anything. Just indicates end of TX
        cmdtxend.set_cmd_id(protobufs::Command::TxCommit);
        cmdtxend.set_tx_id(txid);
        cmds.push_back(&cmdtxend);

        vector<protobufs::CommandResponse *> responses = qh.process_queries(cmds);
        int nodecount, propcount = 0;
        for (auto it : responses) {
            ASSERT_EQ(it->error_code(), protobufs::CommandResponse::Success) << "Unsuccessful TX";
            if (it->r_type() == protobufs::List) {
                auto mymap = it->prop_values();
                for(auto m_it : mymap) {
                    // Assuming string for now
                    protobufs::PropertyList &p = m_it.second;
                    nodecount = 0;
                    for (int i = 0; i < p.values_size(); ++i) {
                        print_property(m_it.first, p.values(i));
                        nodecount++;
                    }
                    propcount++;
                }
            }
            printf("\n");
        }
        EXPECT_EQ(nodecount, 2) << "Not enough nodes found";
        EXPECT_EQ(propcount, 2) << "Not enough properties read";
    }
}
