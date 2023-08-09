/**
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

#include "gtest/gtest.h"

#include <mutex>
#include <vector>

#include "PMGDQueryHandler.h"
#include "VDMSConfig.h"
#include "pmgd.h"
#include "pmgdMessages.pb.h" // Protobuff implementation

#include <stdlib.h> /* system, NULL, EXIT_FAILURE */

using namespace PMGD;
using namespace VDMS;
using namespace std;

#define MALE 0
#define FEMALE 1

void add_patient(protobufs::Command &cmdadd, int id, string name, int age,
                 string dob, string email, int sex) {
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
  p = n->add_properties();
  p->set_type(protobufs::Property::StringType);
  p->set_key("RemoveViaUpdate");
  p->set_string_value("Random");
}

TEST(PMGDQueryHandler, addTest) {
  VDMSConfig::init("unit_tests/config-pmgd-tests.json");
  PMGDQueryHandler::init();
  PMGDQueryHandler qh;

  vector<protobufs::Command *> cmds;

  {
    int txid = 1, patientid = 1, eid = 1, query_count = 0;
    protobufs::Command cmdtx;
    cmdtx.set_cmd_id(protobufs::Command::TxBegin);
    cmdtx.set_tx_id(txid);
    cmds.push_back(&cmdtx);
    query_count++;

    protobufs::Command cmdadd;
    cmdadd.set_tx_id(txid);
    add_patient(cmdadd, patientid++, "John Doe", 86,
                "Sat Nov 1 18:59:24 PDT 1930", "john.doe@abc.com", MALE);
    cmds.push_back(&cmdadd);
    query_count++;

    protobufs::Command cmdadd1;
    cmdadd1.set_tx_id(txid);
    add_patient(cmdadd1, patientid++, "Jane Doe", 80,
                "Sat Oct 1 17:59:24 PDT 1936", "jane.doe@abc.com", FEMALE);
    cmds.push_back(&cmdadd1);
    query_count++;

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
    p = e->add_properties();
    p->set_type(protobufs::Property::StringType);
    p->set_key("Status");
    p->set_string_value("Old Adult");
    cmds.push_back(&cmdedge1);
    query_count++;

    protobufs::Command cmdadd2;
    cmdadd2.set_tx_id(txid);
    add_patient(cmdadd2, patientid++, "Alice Crypto", 70,
                "Sat Nov 1 17:59:24 PDT 1946", "alice.crypto@xyz.com", FEMALE);
    cmds.push_back(&cmdadd2);
    query_count++;

    protobufs::Command cmdadd3;
    cmdadd3.set_tx_id(txid);
    add_patient(cmdadd3, patientid++, "Bob Crypto", 70,
                "Sat Nov 30 7:59:24 PDT 1946", "bob.crypto@xyz.com", MALE);
    cmds.push_back(&cmdadd3);
    query_count++;

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
    p = e->add_properties();
    p->set_type(protobufs::Property::StringType);
    p->set_key("Status");
    p->set_string_value("Old Adult");
    cmds.push_back(&cmdedge2);
    query_count++;

    protobufs::Command cmdtxcommit;
    cmdtxcommit.set_cmd_id(protobufs::Command::TxCommit);
    cmdtxcommit.set_tx_id(txid);
    cmds.push_back(&cmdtxcommit);
    query_count++;

    vector<vector<protobufs::CommandResponse *>> responses =
        qh.process_queries(cmds, query_count, false);
    int nodeids = 1, edgeids = 1;
    for (int i = 0; i < query_count; ++i) {
      vector<protobufs::CommandResponse *> response = responses[i];
      for (auto it : response) {
        EXPECT_EQ(it->error_code(), protobufs::CommandResponse::Success)
            << "Unsuccessful TX";
        if (it->r_type() == protobufs::NodeID) {
          long nodeid = it->op_int_value();
          EXPECT_EQ(nodeid, nodeids++) << "Unexpected node id";
        } else if (it->r_type() == protobufs::EdgeID) {
          long edgeid = it->op_int_value();
          EXPECT_EQ(edgeid, edgeids++) << "Unexpected edge id";
        }
      }
    }
  }
  VDMSConfig::destroy();
  PMGDQueryHandler::destroy();
}

void print_property(const string &key, const protobufs::Property &p) {
#ifdef PRINT_PROPERTY
  switch (p.type()) {
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
#endif
}

TEST(PMGDQueryHandler, queryTestList) {
  VDMSConfig::init("unit_tests/config-pmgd-tests.json");
  PMGDQueryHandler::init();
  PMGDQueryHandler qh;

  vector<protobufs::Command *> cmds;

  {
    int txid = 1, query_count = 0;
    protobufs::Command cmdtx;
    cmdtx.set_cmd_id(protobufs::Command::TxBegin);
    cmdtx.set_tx_id(txid);
    cmds.push_back(&cmdtx);
    query_count++;

    protobufs::Command cmdquery;
    cmdquery.set_cmd_id(protobufs::Command::QueryNode);
    cmdquery.set_tx_id(txid);
    protobufs::QueryNode *qn = cmdquery.mutable_query_node();
    protobufs::Constraints *qc = qn->mutable_constraints();
    protobufs::ResultInfo *qr = qn->mutable_results();
    qn->set_identifier(-1);
    qc->set_tag("Patient");
    qc->set_p_op(protobufs::And);
    protobufs::PropertyPredicate *pp = qc->add_predicates();
    pp->set_key("Email");
    pp->set_op(protobufs::PropertyPredicate::Gt);
    protobufs::Property *p = pp->mutable_v1();
    p->set_type(protobufs::Property::StringType);
    // I think the key is not required here.
    p->set_key("Email");
    p->set_string_value("j");
    qr->set_r_type(protobufs::List);
    string *key = qr->add_response_keys();
    *key = "Email";
    key = qr->add_response_keys();
    *key = "Age";
    cmds.push_back(&cmdquery);
    query_count++;

    // No need to commit in this case. So just end TX
    protobufs::Command cmdtxend;
    // Commit here doesn't change anything. Just indicates end of TX
    cmdtxend.set_cmd_id(protobufs::Command::TxCommit);
    cmdtxend.set_tx_id(txid);
    cmds.push_back(&cmdtxend);
    query_count++;

    vector<vector<protobufs::CommandResponse *>> responses =
        qh.process_queries(cmds, query_count, true);
    int nodecount = 0, propcount = 0;
    for (int q = 0; q < query_count; ++q) {
      vector<protobufs::CommandResponse *> response = responses[q];
      for (auto it : response) {
        EXPECT_EQ(it->error_code(), protobufs::CommandResponse::Success)
            << it->error_msg();
        if (it->r_type() == protobufs::List) {
          auto mymap = it->prop_values();
          for (auto m_it : mymap) {
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
        // printf("\n");
      }
    }
    EXPECT_EQ(nodecount, 2) << "Not enough nodes found";
    EXPECT_EQ(propcount, 2) << "Not enough properties read";
  }
  VDMSConfig::destroy();
  PMGDQueryHandler::destroy();
}

TEST(PMGDQueryHandler, queryTestAverage) {
  VDMSConfig::init("unit_tests/config-pmgd-tests.json");
  PMGDQueryHandler::init();
  PMGDQueryHandler qh;

  vector<protobufs::Command *> cmds;

  {
    int txid = 1, query_count = 0;
    protobufs::Command cmdtx;
    cmdtx.set_cmd_id(protobufs::Command::TxBegin);
    cmdtx.set_tx_id(txid);
    cmds.push_back(&cmdtx);
    query_count++;

    protobufs::Command cmdquery;
    cmdquery.set_cmd_id(protobufs::Command::QueryNode);
    cmdquery.set_tx_id(txid);
    protobufs::QueryNode *qn = cmdquery.mutable_query_node();
    protobufs::Constraints *qc = qn->mutable_constraints();
    protobufs::ResultInfo *qr = qn->mutable_results();
    qn->set_identifier(-1);
    qc->set_tag("Patient");
    qr->set_r_type(protobufs::Average);
    string *key = qr->add_response_keys();
    *key = "Age";
    cmds.push_back(&cmdquery);
    query_count++;

    // No need to commit in this case. So just end TX
    protobufs::Command cmdtxend;
    // Commit here doesn't change anything. Just indicates end of TX
    cmdtxend.set_cmd_id(protobufs::Command::TxCommit);
    cmdtxend.set_tx_id(txid);
    cmds.push_back(&cmdtxend);
    query_count++;

    vector<vector<protobufs::CommandResponse *>> responses =
        qh.process_queries(cmds, query_count, true);
    for (int i = 0; i < query_count; ++i) {
      vector<protobufs::CommandResponse *> response = responses[i];
      for (auto it : response) {
        EXPECT_EQ(it->error_code(), protobufs::CommandResponse::Success)
            << it->error_msg();
        if (it->r_type() == protobufs::Average) {
          EXPECT_EQ(it->op_float_value(), 76.5)
              << "Average didn't match expected for four patients' age";
        }
      }
    }
  }
  VDMSConfig::destroy();
  PMGDQueryHandler::destroy();
}

TEST(PMGDQueryHandler, queryTestUnique) {
  VDMSConfig::init("unit_tests/config-pmgd-tests.json");
  PMGDQueryHandler::init();
  PMGDQueryHandler qh;

  vector<protobufs::Command *> cmds;

  {
    int txid = 1, query_count = 0;
    protobufs::Command cmdtx;
    cmdtx.set_cmd_id(protobufs::Command::TxBegin);
    cmdtx.set_tx_id(txid);
    cmdtx.set_cmd_grp_id(query_count);
    cmds.push_back(&cmdtx);
    query_count++;

    protobufs::Command cmdquery;
    cmdquery.set_cmd_id(protobufs::Command::QueryNode);
    cmdquery.set_tx_id(txid);
    cmdquery.set_cmd_grp_id(query_count);
    protobufs::QueryNode *qn = cmdquery.mutable_query_node();
    protobufs::Constraints *qc = qn->mutable_constraints();
    protobufs::ResultInfo *qr = qn->mutable_results();
    qn->set_identifier(-1);
    qc->set_tag("Patient");
    qc->set_p_op(protobufs::And);
    qc->set_unique(true);
    protobufs::PropertyPredicate *pp = qc->add_predicates();
    pp->set_key("Email");
    pp->set_op(protobufs::PropertyPredicate::Gt);
    protobufs::Property *p = pp->mutable_v1();
    p->set_type(protobufs::Property::StringType);
    // I think the key is not required here.
    p->set_key("Email");
    p->set_string_value("j");
    qr->set_r_type(protobufs::List);
    string *key = qr->add_response_keys();
    *key = "Email";
    cmds.push_back(&cmdquery);
    query_count++;

    // No need to commit in this case. So just end TX
    protobufs::Command cmdtxend;
    // Commit here doesn't change anything. Just indicates end of TX
    cmdtxend.set_cmd_id(protobufs::Command::TxCommit);
    cmdtxend.set_tx_id(txid);
    cmdtxend.set_cmd_grp_id(0);
    cmds.push_back(&cmdtxend);
    query_count++;

    vector<vector<protobufs::CommandResponse *>> responses =
        qh.process_queries(cmds, query_count, true);
    EXPECT_EQ(responses.size(), 1) << "Expecting an error return situation";
    for (int i = 0; i < responses.size(); ++i) {
      vector<protobufs::CommandResponse *> response = responses[i];
      for (auto it : response) {
        if (i == 0) // that's the unique query test
          EXPECT_EQ(it->error_code(), protobufs::CommandResponse::NotUnique)
              << "Was expecting the not unique msg";
      }
    }
  }
  VDMSConfig::destroy();
  PMGDQueryHandler::destroy();
}

TEST(PMGDQueryHandler, queryNeighborTestList) {
  VDMSConfig::init("unit_tests/config-pmgd-tests.json");
  PMGDQueryHandler::init();
  PMGDQueryHandler qh;

  vector<protobufs::Command *> cmds;

  {
    int txid = 1, query_count = 0;
    protobufs::Command cmdtx;
    cmdtx.set_cmd_id(protobufs::Command::TxBegin);
    cmdtx.set_tx_id(txid);
    cmds.push_back(&cmdtx);
    query_count++;

    // Set parameters to find the starting node(s)
    protobufs::Command cmdstartquery;
    cmdstartquery.set_cmd_id(protobufs::Command::QueryNode);
    cmdstartquery.set_tx_id(txid);
    protobufs::QueryNode *qn = cmdstartquery.mutable_query_node();
    protobufs::Constraints *qc = qn->mutable_constraints();
    protobufs::ResultInfo *qr = qn->mutable_results();
    qn->set_identifier(1);
    qc->set_tag("Patient");
    qc->set_p_op(protobufs::And);
    protobufs::PropertyPredicate *pp = qc->add_predicates();
    pp->set_key("Sex");
    pp->set_op(protobufs::PropertyPredicate::Eq);
    protobufs::Property *p = pp->mutable_v1();
    p->set_type(protobufs::Property::IntegerType);
    // I think the key is not required here.
    p->set_key("Sex");
    p->set_int_value(MALE);
    cmds.push_back(&cmdstartquery);
    query_count++;

    protobufs::Command cmdquery;
    cmdquery.set_cmd_id(protobufs::Command::QueryNode);
    cmdquery.set_tx_id(txid);
    qn = cmdquery.mutable_query_node();
    qc = qn->mutable_constraints();
    qr = qn->mutable_results();
    qn->set_identifier(-1);
    protobufs::LinkInfo *qnb = qn->mutable_link();
    // Now set parameters for neighbor traversal
    qnb->set_start_identifier(1);
    qnb->set_e_tag("Married");
    qnb->set_dir(protobufs::LinkInfo::Any);
    qnb->set_nb_unique(false);

    qc->set_p_op(protobufs::And);
    qc->set_tagid(0);
    qc->set_unique(false);
    qr->set_r_type(protobufs::List);
    string *key = qr->add_response_keys();
    *key = "Name";
    cmds.push_back(&cmdquery);
    query_count++;

    // No need to commit in this case. So just end TX
    protobufs::Command cmdtxend;
    // Commit here doesn't change anything. Just indicates end of TX
    cmdtxend.set_cmd_id(protobufs::Command::TxCommit);
    cmdtxend.set_tx_id(txid);
    cmds.push_back(&cmdtxend);
    query_count++;

    vector<vector<protobufs::CommandResponse *>> responses =
        qh.process_queries(cmds, query_count, true);
    int nodecount = 0, propcount = 0;
    for (int q = 0; q < query_count; ++q) {
      vector<protobufs::CommandResponse *> response = responses[q];
      for (auto it : response) {
        EXPECT_EQ(it->error_code(), protobufs::CommandResponse::Success)
            << it->error_msg();
        if (it->r_type() == protobufs::List) {
          auto mymap = it->prop_values();
          for (auto m_it : mymap) {
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
        // printf("\n");
      }
    }
    EXPECT_EQ(nodecount, 2) << "Not enough nodes found";
    EXPECT_EQ(propcount, 1) << "Not enough properties read";
  }
  VDMSConfig::destroy();
  PMGDQueryHandler::destroy();
}

TEST(PMGDQueryHandler, queryConditionalNeighborTestList) {
  VDMSConfig::init("unit_tests/config-pmgd-tests.json");
  PMGDQueryHandler::init();
  PMGDQueryHandler qh;

  vector<protobufs::Command *> cmds;

  {
    int txid = 1, query_count = 0;
    protobufs::Command cmdtx;
    cmdtx.set_cmd_id(protobufs::Command::TxBegin);
    cmdtx.set_tx_id(txid);
    cmds.push_back(&cmdtx);
    query_count++;

    // Set parameters to find the starting node(s)
    protobufs::Command cmdstartquery;
    cmdstartquery.set_cmd_id(protobufs::Command::QueryNode);
    cmdstartquery.set_tx_id(txid);
    protobufs::QueryNode *qn = cmdstartquery.mutable_query_node();
    protobufs::Constraints *qc = qn->mutable_constraints();
    protobufs::ResultInfo *qr = qn->mutable_results();
    qn->set_identifier(1);
    qc->set_tag("Patient");
    qc->set_p_op(protobufs::And);
    protobufs::PropertyPredicate *pp = qc->add_predicates();
    pp->set_key("Sex");
    pp->set_op(protobufs::PropertyPredicate::Eq);
    protobufs::Property *p = pp->mutable_v1();
    p->set_type(protobufs::Property::IntegerType);
    // I think the key is not required here.
    p->set_key("Sex");
    p->set_int_value(MALE);
    cmds.push_back(&cmdstartquery);
    query_count++;

    protobufs::Command cmdquery;
    cmdquery.set_cmd_id(protobufs::Command::QueryNode);
    cmdquery.set_tx_id(txid);
    qn = cmdquery.mutable_query_node();
    qc = qn->mutable_constraints();
    qr = qn->mutable_results();
    qn->set_identifier(-1);
    protobufs::LinkInfo *qnb = qn->mutable_link();
    // Now set parameters for neighbor traversal
    qnb->set_start_identifier(1);
    qnb->set_e_tag("Married");
    qnb->set_dir(protobufs::LinkInfo::Any);
    qnb->set_nb_unique(false);

    qc->set_tag("Patient");
    qc->set_p_op(protobufs::And);
    pp = qc->add_predicates();
    pp->set_key("Age");
    pp->set_op(protobufs::PropertyPredicate::Lt);
    p = pp->mutable_v1();
    p->set_type(protobufs::Property::IntegerType);
    // I think the key is not required here.
    p->set_key("Age");
    p->set_int_value(80);

    qc->set_unique(false);
    qr->set_r_type(protobufs::List);
    string *key = qr->add_response_keys();
    *key = "Name";
    cmds.push_back(&cmdquery);
    query_count++;

    // No need to commit in this case. So just end TX
    protobufs::Command cmdtxend;
    // Commit here doesn't change anything. Just indicates end of TX
    cmdtxend.set_cmd_id(protobufs::Command::TxCommit);
    cmdtxend.set_tx_id(txid);
    cmds.push_back(&cmdtxend);
    query_count++;

    vector<vector<protobufs::CommandResponse *>> responses =
        qh.process_queries(cmds, query_count, true);
    int nodecount = 0, propcount = 0;
    for (int q = 0; q < query_count; ++q) {
      vector<protobufs::CommandResponse *> response = responses[q];
      for (auto it : response) {
        EXPECT_EQ(it->error_code(), protobufs::CommandResponse::Success)
            << it->error_msg();
        if (it->r_type() == protobufs::List) {
          auto mymap = it->prop_values();
          for (auto m_it : mymap) {
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
        // printf("\n");
      }
    }
    EXPECT_EQ(nodecount, 1) << "Not enough nodes found";
    EXPECT_EQ(propcount, 1) << "Not enough properties read";
  }
  VDMSConfig::destroy();
  PMGDQueryHandler::destroy();
}

TEST(PMGDQueryHandler, queryNeighborTestSum) {
  VDMSConfig::init("unit_tests/config-pmgd-tests.json");
  PMGDQueryHandler::init();
  PMGDQueryHandler qh;

  vector<protobufs::Command *> cmds;

  {
    int txid = 1, query_count = 0;
    protobufs::Command cmdtx;
    cmdtx.set_cmd_id(protobufs::Command::TxBegin);
    cmdtx.set_tx_id(txid);
    cmds.push_back(&cmdtx);
    query_count++;

    // Set parameters to find the starting node(s)
    protobufs::Command cmdstartquery;
    cmdstartquery.set_cmd_id(protobufs::Command::QueryNode);
    cmdstartquery.set_tx_id(txid);
    protobufs::QueryNode *qn = cmdstartquery.mutable_query_node();
    protobufs::Constraints *qc = qn->mutable_constraints();
    protobufs::ResultInfo *qr = qn->mutable_results();
    // Set parameters to find the starting node(s)
    qn->set_identifier(1);
    qc->set_tag("Patient");
    qc->set_p_op(protobufs::And);
    qc->set_unique(false);
    protobufs::PropertyPredicate *pp = qc->add_predicates();
    pp->set_key("Sex");
    pp->set_op(protobufs::PropertyPredicate::Eq);
    protobufs::Property *p = pp->mutable_v1();
    p->set_type(protobufs::Property::IntegerType);
    // I think the key is not required here.
    p->set_key("Sex");
    p->set_int_value(MALE);
    cmds.push_back(&cmdstartquery);
    query_count++;

    protobufs::Command cmdquery;
    cmdquery.set_cmd_id(protobufs::Command::QueryNode);
    cmdquery.set_tx_id(txid);
    qn = cmdquery.mutable_query_node();
    qc = qn->mutable_constraints();
    qr = qn->mutable_results();
    qn->set_identifier(-1);
    protobufs::LinkInfo *qnb = qn->mutable_link();
    // Now set parameters for neighbor traversal
    qnb->set_start_identifier(1);
    qnb->set_e_tag("Married");
    qnb->set_dir(protobufs::LinkInfo::Any);
    qnb->set_nb_unique(false);
    qc->set_tag("Patient");
    qc->set_p_op(protobufs::And);
    qr->set_r_type(protobufs::Sum);
    string *key = qr->add_response_keys();
    *key = "Age";
    cmds.push_back(&cmdquery);
    query_count++;

    // No need to commit in this case. So just end TX
    protobufs::Command cmdtxend;
    // Commit here doesn't change anything. Just indicates end of TX
    cmdtxend.set_cmd_id(protobufs::Command::TxCommit);
    cmdtxend.set_tx_id(txid);
    cmds.push_back(&cmdtxend);
    query_count++;

    vector<vector<protobufs::CommandResponse *>> responses =
        qh.process_queries(cmds, query_count, true);
    int nodecount = 0, propcount = 0;
    for (int i = 0; i < query_count; ++i) {
      vector<protobufs::CommandResponse *> response = responses[i];
      for (auto it : response) {
        EXPECT_EQ(it->error_code(), protobufs::CommandResponse::Success)
            << it->error_msg();
        if (it->r_type() == protobufs::Sum) {
          EXPECT_EQ(it->op_int_value(), 150)
              << "Sum didn't match expected for two patients' age";
        }
      }
    }
  }
  VDMSConfig::destroy();
  PMGDQueryHandler::destroy();
}

TEST(PMGDQueryHandler, addConstrainedTest) {
  VDMSConfig::init("unit_tests/config-pmgd-tests.json");
  PMGDQueryHandler::init();
  PMGDQueryHandler qh;

  vector<protobufs::Command *> cmds;

  {
    int txid = 1, patientid = 1, eid = 1, query_count = 0;
    protobufs::Command cmdtx;
    cmdtx.set_cmd_id(protobufs::Command::TxBegin);
    cmdtx.set_tx_id(txid);
    cmdtx.set_cmd_grp_id(query_count);
    cmds.push_back(&cmdtx);
    query_count++;

    protobufs::Command cmdadd;
    cmdadd.set_tx_id(txid);
    cmdadd.set_cmd_grp_id(query_count);
    add_patient(cmdadd, patientid, "John Doe", 86,
                "Sat Nov 1 18:59:24 PDT 1930", "john.doe@abc.com", MALE);
    // Add a test to verify this node doesn't exist
    protobufs::AddNode *an = cmdadd.mutable_add_node();
    protobufs::QueryNode *qn = an->mutable_query_node();
    protobufs::Constraints *qc = qn->mutable_constraints();
    protobufs::ResultInfo *qr = qn->mutable_results();
    qn->set_identifier(patientid++); // ref for caching in case found.
    qc->set_tag("Patient");
    qc->set_unique(true);
    qc->set_p_op(protobufs::And);
    qr->set_r_type(protobufs::NodeID);
    protobufs::PropertyPredicate *pp = qc->add_predicates();
    pp->set_key("Email");
    pp->set_op(protobufs::PropertyPredicate::Eq);
    protobufs::Property *p = pp->mutable_v1();
    p->set_type(protobufs::Property::StringType);
    // I think the key is not required here.
    p->set_key("Email");
    p->set_string_value("john.doe@abc.com");
    cmds.push_back(&cmdadd);
    query_count++;

    protobufs::Command cmdadd1;
    cmdadd1.set_tx_id(txid);
    cmdadd1.set_cmd_grp_id(query_count);
    add_patient(cmdadd1, patientid++, "Janice Doe", 40,
                "Fri Oct 1 1:59:24 PDT 1976", "janice.doe@abc.com", FEMALE);
    cmds.push_back(&cmdadd1);
    query_count++;

    protobufs::Command cmdedge1;
    cmdedge1.set_tx_id(txid);
    cmdedge1.set_cmd_id(protobufs::Command::AddEdge);
    cmdedge1.set_cmd_grp_id(query_count);
    protobufs::AddEdge *ae = cmdedge1.mutable_add_edge();
    ae->set_identifier(eid++);
    protobufs::Edge *e = ae->mutable_edge();
    e->set_src(1);
    e->set_dst(2);
    e->set_tag("Daughter");
    p = e->add_properties();
    p->set_type(protobufs::Property::StringType);
    p->set_key("Status");
    p->set_string_value("Young Adult");
    cmds.push_back(&cmdedge1);
    query_count++;

    protobufs::Command cmdtxcommit;
    cmdtxcommit.set_cmd_id(protobufs::Command::TxCommit);
    cmdtxcommit.set_tx_id(txid);
    cmdtxcommit.set_cmd_grp_id(0);
    cmds.push_back(&cmdtxcommit);
    query_count++;

    vector<vector<protobufs::CommandResponse *>> responses =
        qh.process_queries(cmds, query_count, false);

    // Since PMGD queries always generate one response per command,
    // we can do the following:
    protobufs::CommandResponse *resp = responses[0][0]; // TxBegin
    EXPECT_EQ(resp->error_code(), protobufs::CommandResponse::Success)
        << "Unsuccessful TX";
    resp = responses[1][0]; // Conditional add
    EXPECT_EQ(resp->error_code(), protobufs::CommandResponse::Exists)
        << resp->error_msg();
    EXPECT_EQ(resp->op_int_value(), 1)
        << "Unexpected node id for conditional add";
    resp = responses[2][0]; // Regular add
    EXPECT_EQ(resp->error_code(), protobufs::CommandResponse::Success)
        << resp->error_msg();
    EXPECT_EQ(resp->op_int_value(), 5) << "Unexpected node id for add";
    resp = responses[3][0]; // Regular add edge
    EXPECT_EQ(resp->error_code(), protobufs::CommandResponse::Success)
        << resp->error_msg();
    EXPECT_EQ(resp->op_int_value(), 3) << "Unexpected edge id for add";
  }
  VDMSConfig::destroy();
  PMGDQueryHandler::destroy();
}

TEST(PMGDQueryHandler, queryNeighborLinksTestList) {
  VDMSConfig::init("unit_tests/config-pmgd-tests.json");
  PMGDQueryHandler::init();
  PMGDQueryHandler qh;

  vector<protobufs::Command *> cmds;

  {
    int txid = 1, query_count = 0;
    protobufs::Command cmdtx;
    cmdtx.set_cmd_id(protobufs::Command::TxBegin);
    cmdtx.set_tx_id(txid);
    cmds.push_back(&cmdtx);
    query_count++;

    // Set parameters to find the starting node(s)
    protobufs::Command cmdstartquery;
    cmdstartquery.set_cmd_id(protobufs::Command::QueryNode);
    cmdstartquery.set_tx_id(txid);
    protobufs::QueryNode *qn = cmdstartquery.mutable_query_node();
    protobufs::Constraints *qc = qn->mutable_constraints();
    protobufs::ResultInfo *qr = qn->mutable_results();
    qn->set_identifier(1);
    qc->set_tag("Patient");
    qc->set_p_op(protobufs::And);
    protobufs::PropertyPredicate *pp = qc->add_predicates();
    pp->set_key("Sex");
    pp->set_op(protobufs::PropertyPredicate::Eq);
    protobufs::Property *p = pp->mutable_v1();
    p->set_type(protobufs::Property::IntegerType);
    // I think the key is not required here.
    p->set_key("Sex");
    p->set_int_value(FEMALE);
    cmds.push_back(&cmdstartquery);
    query_count++;

    protobufs::Command cmdquery;
    cmdquery.set_cmd_id(protobufs::Command::QueryNode);
    cmdquery.set_tx_id(txid);
    qn = cmdquery.mutable_query_node();
    qc = qn->mutable_constraints();
    qr = qn->mutable_results();
    qn->set_identifier(2);
    protobufs::LinkInfo *qnb = qn->mutable_link();
    // Now set parameters for neighbor traversal
    qnb->set_start_identifier(1);
    qnb->set_e_tag("Married");
    qnb->set_dir(protobufs::LinkInfo::Any);
    qnb->set_nb_unique(false);
    qc->set_tagid(0);
    qc->set_unique(false);
    qc->set_p_op(protobufs::And);
    cmds.push_back(&cmdquery);
    query_count++;

    protobufs::Command cmdfollquery;
    cmdfollquery.set_cmd_id(protobufs::Command::QueryNode);
    cmdfollquery.set_tx_id(txid);
    qn = cmdfollquery.mutable_query_node();
    qc = qn->mutable_constraints();
    qr = qn->mutable_results();
    qn->set_identifier(-1);
    qnb = qn->mutable_link();
    // Now set parameters for neighbor traversal
    qnb->set_start_identifier(2);
    qnb->set_e_tag("Daughter");
    qnb->set_dir(protobufs::LinkInfo::Any);
    qnb->set_nb_unique(false);
    qc->set_tagid(0);
    qc->set_unique(false);
    qc->set_p_op(protobufs::And);
    qr->set_r_type(protobufs::List);
    string *key = qr->add_response_keys();
    *key = "Name";
    cmds.push_back(&cmdfollquery);
    query_count++;

    // No need to commit in this case. So just end TX
    protobufs::Command cmdtxend;
    // Commit here doesn't change anything. Just indicates end of TX
    cmdtxend.set_cmd_id(protobufs::Command::TxCommit);
    cmdtxend.set_tx_id(txid);
    cmds.push_back(&cmdtxend);
    query_count++;

    vector<vector<protobufs::CommandResponse *>> responses =
        qh.process_queries(cmds, query_count, true);
    int nodecount = 0, propcount = 0;
    for (int q = 0; q < query_count; ++q) {
      vector<protobufs::CommandResponse *> response = responses[q];
      for (auto it : response) {
        EXPECT_EQ(it->error_code(), protobufs::CommandResponse::Success)
            << it->error_msg();
        if (it->r_type() == protobufs::List) {
          auto mymap = it->prop_values();
          for (auto m_it : mymap) {
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
        // printf("\n");
      }
    }
    EXPECT_EQ(nodecount, 1) << "Not enough nodes found";
    EXPECT_EQ(propcount, 1) << "Not enough properties read";
  }
  VDMSConfig::destroy();
  PMGDQueryHandler::destroy();
}

TEST(PMGDQueryHandler, queryNeighborLinksReuseTestList) {
  VDMSConfig::init("unit_tests/config-pmgd-tests.json");
  PMGDQueryHandler::init();
  PMGDQueryHandler qh;

  vector<protobufs::Command *> cmds;

  {
    int txid = 1, query_count = 0;
    protobufs::Command cmdtx;
    cmdtx.set_cmd_id(protobufs::Command::TxBegin);
    cmdtx.set_tx_id(txid);
    cmds.push_back(&cmdtx);
    query_count++;

    // Set parameters to find the starting node(s)
    protobufs::Command cmdstartquery;
    cmdstartquery.set_cmd_id(protobufs::Command::QueryNode);
    cmdstartquery.set_tx_id(txid);
    protobufs::QueryNode *qn = cmdstartquery.mutable_query_node();
    protobufs::Constraints *qc = qn->mutable_constraints();
    protobufs::ResultInfo *qr = qn->mutable_results();
    qn->set_identifier(1);
    qc->set_tag("Patient");
    qc->set_p_op(protobufs::And);
    protobufs::PropertyPredicate *pp = qc->add_predicates();
    pp->set_key("Sex");
    pp->set_op(protobufs::PropertyPredicate::Eq);
    protobufs::Property *p = pp->mutable_v1();
    p->set_type(protobufs::Property::IntegerType);
    // I think the key is not required here.
    p->set_key("Sex");
    p->set_int_value(FEMALE);
    qr->set_r_type(protobufs::List);
    string *key = qr->add_response_keys();
    *key = "Email";
    cmds.push_back(&cmdstartquery);
    query_count++;

    protobufs::Command cmdquery;
    cmdquery.set_cmd_id(protobufs::Command::QueryNode);
    cmdquery.set_tx_id(txid);
    qn = cmdquery.mutable_query_node();
    qc = qn->mutable_constraints();
    qr = qn->mutable_results();
    qn->set_identifier(2);
    protobufs::LinkInfo *qnb = qn->mutable_link();
    // Now set parameters for neighbor traversal
    qnb->set_start_identifier(1);
    qnb->set_e_tag("Married");
    qnb->set_dir(protobufs::LinkInfo::Any);
    qnb->set_nb_unique(false);
    qc->set_tagid(0);
    qc->set_unique(false);
    qc->set_p_op(protobufs::And);
    qr->set_r_type(protobufs::Count);
    cmds.push_back(&cmdquery);
    query_count++;

    protobufs::Command cmdfollquery;
    cmdfollquery.set_cmd_id(protobufs::Command::QueryNode);
    cmdfollquery.set_tx_id(txid);
    qn = cmdfollquery.mutable_query_node();
    qc = qn->mutable_constraints();
    qr = qn->mutable_results();
    qn->set_identifier(-1);
    qnb = qn->mutable_link();
    // Now set parameters for neighbor traversal
    qnb->set_start_identifier(2);
    qnb->set_e_tag("Daughter");
    qnb->set_dir(protobufs::LinkInfo::Any);
    qnb->set_nb_unique(false);
    qc->set_tagid(0);
    qc->set_unique(false);
    qc->set_p_op(protobufs::And);
    qr->set_r_type(protobufs::List);
    key = qr->add_response_keys();
    *key = "Name";
    key = qr->add_response_keys();
    *key = "Email";
    cmds.push_back(&cmdfollquery);
    query_count++;

    // No need to commit in this case. So just end TX
    protobufs::Command cmdtxend;
    // Commit here doesn't change anything. Just indicates end of TX
    cmdtxend.set_cmd_id(protobufs::Command::TxCommit);
    cmdtxend.set_tx_id(txid);
    cmds.push_back(&cmdtxend);
    query_count++;

    vector<vector<protobufs::CommandResponse *>> responses =
        qh.process_queries(cmds, query_count, true);
    int nodecount = 0, propcount = 0;
    int totnodecount = 0, totpropcount = 0;
    for (int q = 0; q < query_count; ++q) {
      vector<protobufs::CommandResponse *> response = responses[q];
      for (auto it : response) {
        EXPECT_EQ(it->error_code(), protobufs::CommandResponse::Success)
            << it->error_msg();
        if (it->r_type() == protobufs::List) {
          propcount = 0;
          auto mymap = it->prop_values();
          for (auto m_it : mymap) {
            // Assuming string for now
            protobufs::PropertyList &p = m_it.second;
            nodecount = 0;
            propcount++;
            for (int i = 0; i < p.values_size(); ++i) {
              print_property(m_it.first, p.values(i));
              nodecount++;
            }
          }
          totpropcount += propcount;
          totnodecount += nodecount;
        }
        if (it->r_type() == protobufs::Count) {
          EXPECT_EQ(it->op_int_value(), 2) << "Doesn't match expected count";
        }
        // printf("\n");
      }
    }
    EXPECT_EQ(nodecount, 1) << "Not enough nodes found";
    EXPECT_EQ(propcount, 2) << "Not enough properties read";
    EXPECT_EQ(totnodecount, 4) << "Not enough total nodes found";
    EXPECT_EQ(totpropcount, 3) << "Not enough total properties read";
  }
  VDMSConfig::destroy();
  PMGDQueryHandler::destroy();
}

TEST(PMGDQueryHandler, querySortedNeighborLinksReuseTestList) {
  VDMSConfig::init("unit_tests/config-pmgd-tests.json");
  PMGDQueryHandler::init();
  PMGDQueryHandler qh;

  vector<protobufs::Command *> cmds;

  {
    int txid = 1, query_count = 0;
    protobufs::Command cmdtx;
    cmdtx.set_cmd_id(protobufs::Command::TxBegin);
    cmdtx.set_tx_id(txid);
    cmds.push_back(&cmdtx);
    query_count++;

    // Set parameters to find the starting node(s)
    protobufs::Command cmdstartquery;
    cmdstartquery.set_cmd_id(protobufs::Command::QueryNode);
    cmdstartquery.set_tx_id(txid);
    protobufs::QueryNode *qn = cmdstartquery.mutable_query_node();
    protobufs::Constraints *qc = qn->mutable_constraints();
    protobufs::ResultInfo *qr = qn->mutable_results();
    qn->set_identifier(1);
    qc->set_tag("Patient");
    qc->set_p_op(protobufs::And);
    protobufs::PropertyPredicate *pp = qc->add_predicates();
    pp->set_key("Sex");
    pp->set_op(protobufs::PropertyPredicate::Eq);
    protobufs::Property *p = pp->mutable_v1();
    p->set_type(protobufs::Property::IntegerType);
    // I think the key is not required here.
    p->set_key("Sex");
    p->set_int_value(FEMALE);
    qr->set_r_type(protobufs::List);
    string *key = qr->add_response_keys();
    *key = "Email";
    qr->set_sort(true);
    qr->set_sort_key("Email");
    cmds.push_back(&cmdstartquery);
    query_count++;

    protobufs::Command cmdquery;
    cmdquery.set_cmd_id(protobufs::Command::QueryNode);
    cmdquery.set_tx_id(txid);
    qn = cmdquery.mutable_query_node();
    qc = qn->mutable_constraints();
    qr = qn->mutable_results();
    qn->set_identifier(2);
    protobufs::LinkInfo *qnb = qn->mutable_link();
    // Now set parameters for neighbor traversal
    qnb->set_start_identifier(1);
    qnb->set_e_tag("Married");
    qnb->set_dir(protobufs::LinkInfo::Any);
    qnb->set_nb_unique(false);
    qc->set_tagid(0);
    qc->set_unique(false);
    qc->set_p_op(protobufs::And);
    qr->set_r_type(protobufs::Count);
    cmds.push_back(&cmdquery);
    query_count++;

    protobufs::Command cmdfollquery;
    cmdfollquery.set_cmd_id(protobufs::Command::QueryNode);
    cmdfollquery.set_tx_id(txid);
    qn = cmdfollquery.mutable_query_node();
    qc = qn->mutable_constraints();
    qr = qn->mutable_results();
    qn->set_identifier(-1);
    qnb = qn->mutable_link();
    // Now set parameters for neighbor traversal
    qnb->set_start_identifier(2);
    qnb->set_e_tag("Daughter");
    qnb->set_dir(protobufs::LinkInfo::Any);
    qnb->set_nb_unique(false);
    qc->set_tagid(0);
    qc->set_unique(false);
    qc->set_p_op(protobufs::And);
    qr->set_r_type(protobufs::List);
    key = qr->add_response_keys();
    *key = "Name";
    key = qr->add_response_keys();
    *key = "Email";
    cmds.push_back(&cmdfollquery);
    query_count++;

    // No need to commit in this case. So just end TX
    protobufs::Command cmdtxend;
    // Commit here doesn't change anything. Just indicates end of TX
    cmdtxend.set_cmd_id(protobufs::Command::TxCommit);
    cmdtxend.set_tx_id(txid);
    cmds.push_back(&cmdtxend);
    query_count++;

    vector<vector<protobufs::CommandResponse *>> responses =
        qh.process_queries(cmds, query_count, true);
    int nodecount = 0, propcount = 0;
    int totnodecount = 0, totpropcount = 0;
    bool firstquery = true;
    for (int q = 0; q < query_count; ++q) {
      vector<protobufs::CommandResponse *> response = responses[q];
      for (auto it : response) {
        EXPECT_EQ(it->error_code(), protobufs::CommandResponse::Success)
            << it->error_msg();
        if (it->r_type() == protobufs::List) {
          propcount = 0;
          auto mymap = it->prop_values();
          for (auto m_it : mymap) {
            // Assuming string for now
            protobufs::PropertyList &p = m_it.second;
            nodecount = 0;
            propcount++;
            for (int i = 0; i < p.values_size(); ++i) {
              print_property(m_it.first, p.values(i));
              nodecount++;
            }
            if (firstquery) {
              firstquery = false;
              EXPECT_EQ(p.values(0).string_value(), "alice.crypto@xyz.com")
                  << "Sorting didn't work";
            }
          }
          totpropcount += propcount;
          totnodecount += nodecount;
        }
        if (it->r_type() == protobufs::Count) {
          EXPECT_EQ(it->op_int_value(), 2) << "Doesn't match expected count";
        }
        // printf("\n");
      }
    }
    EXPECT_EQ(nodecount, 1) << "Not enough nodes found";
    EXPECT_EQ(propcount, 2) << "Not enough properties read";
    EXPECT_EQ(totnodecount, 4) << "Not enough total nodes found";
    EXPECT_EQ(totpropcount, 3) << "Not enough total properties read";
  }
  VDMSConfig::destroy();
  PMGDQueryHandler::destroy();
}

TEST(PMGDQueryHandler, queryTestListLimit) {
  VDMSConfig::init("unit_tests/config-pmgd-tests.json");
  PMGDQueryHandler::init();
  PMGDQueryHandler qh;

  vector<protobufs::Command *> cmds;

  {
    int txid = 1, query_count = 0;
    protobufs::Command cmdtx;
    cmdtx.set_cmd_id(protobufs::Command::TxBegin);
    cmdtx.set_tx_id(txid);
    cmds.push_back(&cmdtx);
    query_count++;

    protobufs::Command cmdquery;
    cmdquery.set_cmd_id(protobufs::Command::QueryNode);
    cmdquery.set_tx_id(txid);
    protobufs::QueryNode *qn = cmdquery.mutable_query_node();
    protobufs::Constraints *qc = qn->mutable_constraints();
    protobufs::ResultInfo *qr = qn->mutable_results();
    qn->set_identifier(-1);
    qc->set_tag("Patient");
    qc->set_p_op(protobufs::And);
    qr->set_r_type(protobufs::List);
    string *key = qr->add_response_keys();
    *key = "Email";
    key = qr->add_response_keys();
    *key = "Age";
    qr->set_limit(4);
    cmds.push_back(&cmdquery);
    query_count++;

    // No need to commit in this case. So just end TX
    protobufs::Command cmdtxend;
    // Commit here doesn't change anything. Just indicates end of TX
    cmdtxend.set_cmd_id(protobufs::Command::TxCommit);
    cmdtxend.set_tx_id(txid);
    cmds.push_back(&cmdtxend);
    query_count++;

    vector<vector<protobufs::CommandResponse *>> responses =
        qh.process_queries(cmds, query_count, true);
    int nodecount = 0, propcount = 0;
    for (int q = 0; q < query_count; ++q) {
      vector<protobufs::CommandResponse *> response = responses[q];
      for (auto it : response) {
        EXPECT_EQ(it->error_code(), protobufs::CommandResponse::Success)
            << it->error_msg();
        if (it->r_type() == protobufs::List) {
          auto mymap = it->prop_values();
          for (auto m_it : mymap) {
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
        // printf("\n");
      }
    }
    EXPECT_EQ(nodecount, 4) << "Incorrect number of nodes found";
    EXPECT_EQ(propcount, 2) << "Not enough properties read";
  }
  VDMSConfig::destroy();
  PMGDQueryHandler::destroy();
}

TEST(PMGDQueryHandler, queryTestSortedLimitedAverage) {
  VDMSConfig::init("unit_tests/config-pmgd-tests.json");
  PMGDQueryHandler::init();
  PMGDQueryHandler qh;

  vector<protobufs::Command *> cmds;

  {
    int txid = 1, query_count = 0;
    protobufs::Command cmdtx;
    cmdtx.set_cmd_id(protobufs::Command::TxBegin);
    cmdtx.set_tx_id(txid);
    cmds.push_back(&cmdtx);
    query_count++;

    protobufs::Command cmdquery;
    cmdquery.set_cmd_id(protobufs::Command::QueryNode);
    cmdquery.set_tx_id(txid);
    protobufs::QueryNode *qn = cmdquery.mutable_query_node();
    protobufs::Constraints *qc = qn->mutable_constraints();
    protobufs::ResultInfo *qr = qn->mutable_results();
    qn->set_identifier(-1);
    qc->set_tag("Patient");
    qr->set_r_type(protobufs::Average);
    string *key = qr->add_response_keys();
    *key = "Age";
    qr->set_sort(true);
    qr->set_sort_key("Email");
    // Average over 5 patients age is 69.2
    qr->set_limit(3);
    cmds.push_back(&cmdquery);
    query_count++;

    // No need to commit in this case. So just end TX
    protobufs::Command cmdtxend;
    // Commit here doesn't change anything. Just indicates end of TX
    cmdtxend.set_cmd_id(protobufs::Command::TxCommit);
    cmdtxend.set_tx_id(txid);
    cmds.push_back(&cmdtxend);
    query_count++;

    vector<vector<protobufs::CommandResponse *>> responses =
        qh.process_queries(cmds, query_count, true);
    for (int i = 0; i < query_count; ++i) {
      vector<protobufs::CommandResponse *> response = responses[i];
      for (auto it : response) {
        EXPECT_EQ(it->error_code(), protobufs::CommandResponse::Success)
            << it->error_msg();
        if (it->r_type() == protobufs::Average) {
          EXPECT_EQ(static_cast<int>(it->op_float_value()), 73)
              << "Average didn't match expected for three middle patients' age";
        }
      }
    }
  }
  VDMSConfig::destroy();
  PMGDQueryHandler::destroy();
}

TEST(PMGDQueryHandler, queryUpdateTest) {
  // printf("Testing PMGD query protobuf handler for list return of neighbors
  // with constraints\n");

  VDMSConfig::init("unit_tests/config-pmgd-tests.json");
  PMGDQueryHandler::init();
  PMGDQueryHandler qh;

  vector<protobufs::Command *> cmds;
  {
    int txid = 1, query_count = 0;
    protobufs::Command cmdtx;
    cmdtx.set_cmd_id(protobufs::Command::TxBegin);
    cmdtx.set_tx_id(txid);
    cmds.push_back(&cmdtx);
    query_count++;

    // Set parameters to find the starting node(s)
    protobufs::Command cmdstartquery;
    cmdstartquery.set_cmd_id(protobufs::Command::QueryNode);
    cmdstartquery.set_tx_id(txid);
    protobufs::QueryNode *qn = cmdstartquery.mutable_query_node();
    protobufs::Constraints *qc = qn->mutable_constraints();
    protobufs::ResultInfo *qr = qn->mutable_results();
    qn->set_identifier(1);
    qc->set_tag("Patient");
    qc->set_p_op(protobufs::And);
    protobufs::PropertyPredicate *pp = qc->add_predicates();
    pp->set_key("Sex");
    pp->set_op(protobufs::PropertyPredicate::Eq);
    protobufs::Property *p = pp->mutable_v1();
    p->set_type(protobufs::Property::IntegerType);
    // I think the key is not required here.
    p->set_key("Sex");
    p->set_int_value(MALE);
    cmds.push_back(&cmdstartquery);
    query_count++;

    protobufs::Command cmdupdate;
    cmdupdate.set_cmd_id(protobufs::Command::UpdateNode);
    cmdupdate.set_tx_id(txid);
    protobufs::UpdateNode *un = cmdupdate.mutable_update_node();

    // The identifier here will be the identifier used for search
    // since we are going to update properties of the nodes found
    // in the previous search
    un->set_identifier(qn->identifier());
    p = un->add_properties();
    p->set_type(protobufs::Property::StringType);
    p->set_key("Hospital");
    p->set_string_value("Kaiser1");
    p = un->add_properties();
    p->set_type(protobufs::Property::BooleanType);
    p->set_key("Treated");
    p->set_bool_value(true);

    // Remove the extra properties
    un->add_remove_props("RemoveViaUpdate");

    cmds.push_back(&cmdupdate);
    query_count++;

    // Also make sure the removed property doesn't show up anymore
    protobufs::Command cmdcheckquery;
    cmdcheckquery.set_cmd_id(protobufs::Command::QueryNode);
    cmdcheckquery.set_tx_id(txid);
    qn = cmdcheckquery.mutable_query_node();
    qc = qn->mutable_constraints();
    qr = qn->mutable_results();
    qn->set_identifier(-1);
    qc->set_tag("Patient");
    qc->set_p_op(protobufs::And);
    pp = qc->add_predicates();
    pp->set_key("RemoveViaUpdate");
    pp->set_op(protobufs::PropertyPredicate::Eq);
    p = pp->mutable_v1();
    p->set_type(protobufs::Property::StringType);
    // I think the key is not required here.
    p->set_key("RemoveViaUpdate");
    p->set_string_value("Random");
    qr->set_r_type(protobufs::List);
    string *key = qr->add_response_keys();
    *key = "Email";
    cmds.push_back(&cmdcheckquery);
    query_count++;

    // No need to commit in this case. So just end TX
    protobufs::Command cmdtxend;
    // Commit here doesn't change anything. Just indicates end of TX
    cmdtxend.set_cmd_id(protobufs::Command::TxCommit);
    cmdtxend.set_tx_id(txid);
    cmds.push_back(&cmdtxend);
    query_count++;

    vector<vector<protobufs::CommandResponse *>> responses =
        qh.process_queries(cmds, query_count, false);
    for (int i = 0; i < query_count; ++i) {
      vector<protobufs::CommandResponse *> response = responses[i];
      for (auto it : response) {
        ASSERT_EQ(it->error_code(), protobufs::CommandResponse::Success)
            << it->error_msg();
        if (it->r_type() == protobufs::Count) {
          EXPECT_EQ(it->op_int_value(), 2) << "Doesn't match expected count";
        }
        if (it->r_type() == protobufs::List) {
          EXPECT_EQ(it->op_int_value(), 3)
              << "Doesn't match expected count for prop match";
        }
        // printf("\n");
      }
    }
  }
  VDMSConfig::destroy();
  PMGDQueryHandler::destroy();
}

TEST(PMGDQueryHandler, queryUpdateConstraintTest) {
  // printf("Testing PMGD query protobuf handler for list return of neighbors
  // with constraints\n");

  VDMSConfig::init("unit_tests/config-pmgd-tests.json");
  PMGDQueryHandler::init();
  PMGDQueryHandler qh;

  vector<protobufs::Command *> cmds;
  {
    int txid = 1, query_count = 0;
    protobufs::Command cmdtx;
    cmdtx.set_cmd_id(protobufs::Command::TxBegin);
    cmdtx.set_tx_id(txid);
    cmds.push_back(&cmdtx);
    query_count++;

    // Try with constraints inside the update command
    protobufs::Command cmdupdate;
    cmdupdate.set_cmd_id(protobufs::Command::UpdateNode);
    cmdupdate.set_tx_id(txid);
    protobufs::UpdateNode *un = cmdupdate.mutable_update_node();
    un->set_identifier(1);

    // Set parameters to find the starting node(s)
    protobufs::QueryNode *qn = un->mutable_query_node();
    protobufs::Constraints *qc = qn->mutable_constraints();
    protobufs::ResultInfo *qr = qn->mutable_results();
    qn->set_identifier(un->identifier());
    qc->set_tag("Patient");
    qc->set_p_op(protobufs::And);
    protobufs::PropertyPredicate *pp = qc->add_predicates();
    pp->set_key("Sex");
    pp->set_op(protobufs::PropertyPredicate::Eq);
    protobufs::Property *p = pp->mutable_v1();
    p->set_type(protobufs::Property::IntegerType);
    // I think the key is not required here.
    p->set_key("Sex");
    p->set_int_value(FEMALE);

    // Set properties to be updated when nodes are found.
    p = un->add_properties();
    p->set_type(protobufs::Property::StringType);
    p->set_key("Hospital");
    p->set_string_value("Kaiser2");
    p = un->add_properties();
    p->set_type(protobufs::Property::BooleanType);
    p->set_key("Treated");
    p->set_bool_value(true);

    cmds.push_back(&cmdupdate);
    query_count++;

    // No need to commit in this case. So just end TX
    protobufs::Command cmdtxend;
    // Commit here doesn't change anything. Just indicates end of TX
    cmdtxend.set_cmd_id(protobufs::Command::TxCommit);
    cmdtxend.set_tx_id(txid);
    cmds.push_back(&cmdtxend);
    query_count++;

    vector<vector<protobufs::CommandResponse *>> responses =
        qh.process_queries(cmds, query_count, false);
    for (int i = 0; i < query_count; ++i) {
      vector<protobufs::CommandResponse *> response = responses[i];
      for (auto it : response) {
        ASSERT_EQ(it->error_code(), protobufs::CommandResponse::Success)
            << it->error_msg();
        if (it->r_type() == protobufs::Count) {
          EXPECT_EQ(it->op_int_value(), 3) << "Doesn't match expected count";
        }
        // printf("\n");
      }
    }
  }
  VDMSConfig::destroy();
  PMGDQueryHandler::destroy();
}

TEST(PMGDQueryHandler, queryEdgeTestList) {
  VDMSConfig::init("unit_tests/config-pmgd-tests.json");
  PMGDQueryHandler::init();
  PMGDQueryHandler qh;

  vector<protobufs::Command *> cmds;

  {
    int txid = 1, query_count = 0;
    protobufs::Command cmdtx;
    cmdtx.set_cmd_id(protobufs::Command::TxBegin);
    cmdtx.set_tx_id(txid);
    cmds.push_back(&cmdtx);
    query_count++;

    protobufs::Command cmdquery;
    cmdquery.set_cmd_id(protobufs::Command::QueryEdge);
    cmdquery.set_tx_id(txid);
    protobufs::QueryEdge *qn = cmdquery.mutable_query_edge();
    protobufs::Constraints *qc = qn->mutable_constraints();
    protobufs::ResultInfo *qr = qn->mutable_results();
    qn->set_identifier(-1);
    qc->set_tag("");
    qc->set_p_op(protobufs::And);
    protobufs::PropertyPredicate *pp = qc->add_predicates();
    pp->set_key("Status");
    pp->set_op(protobufs::PropertyPredicate::Eq);
    protobufs::Property *p = pp->mutable_v1();
    p->set_type(protobufs::Property::StringType);
    // I think the key is not required here.
    p->set_key("Status");
    p->set_string_value("Young Adult");
    qr->set_r_type(protobufs::List);
    string *key = qr->add_response_keys();
    *key = "Status";
    cmds.push_back(&cmdquery);
    query_count++;

    // No need to commit in this case. So just end TX
    protobufs::Command cmdtxend;
    // Commit here doesn't change anything. Just indicates end of TX
    cmdtxend.set_cmd_id(protobufs::Command::TxCommit);
    cmdtxend.set_tx_id(txid);
    cmds.push_back(&cmdtxend);
    query_count++;

    vector<vector<protobufs::CommandResponse *>> responses =
        qh.process_queries(cmds, query_count, true);
    int edgecount = 0, propcount = 0;
    for (int q = 0; q < query_count; ++q) {
      vector<protobufs::CommandResponse *> response = responses[q];
      for (auto it : response) {
        ASSERT_EQ(it->error_code(), protobufs::CommandResponse::Success)
            << it->error_msg();
        if (it->r_type() == protobufs::List) {
          auto mymap = it->prop_values();
          for (auto m_it : mymap) {
            // Assuming string for now
            protobufs::PropertyList &p = m_it.second;
            edgecount = 0;
            for (int i = 0; i < p.values_size(); ++i) {
              print_property(m_it.first, p.values(i));
              edgecount++;
            }
            propcount++;
          }
        }
        // printf("\n");
      }
    }
    EXPECT_EQ(edgecount, 1) << "Not enough edges found";
    EXPECT_EQ(propcount, 1) << "Not enough properties read";
  }
  VDMSConfig::destroy();
  PMGDQueryHandler::destroy();
}

TEST(PMGDQueryHandler, queryEdgeTestSortList) {
  // Way to test the reusable iterator
  VDMSConfig::init("unit_tests/config-pmgd-tests.json");
  PMGDQueryHandler::init();
  PMGDQueryHandler qh;

  vector<protobufs::Command *> cmds;

  {
    int txid = 1, query_count = 0;
    protobufs::Command cmdtx;
    cmdtx.set_cmd_id(protobufs::Command::TxBegin);
    cmdtx.set_tx_id(txid);
    cmds.push_back(&cmdtx);
    query_count++;

    protobufs::Command cmdquery;
    cmdquery.set_cmd_id(protobufs::Command::QueryEdge);
    cmdquery.set_tx_id(txid);
    protobufs::QueryEdge *qn = cmdquery.mutable_query_edge();
    protobufs::Constraints *qc = qn->mutable_constraints();
    protobufs::ResultInfo *qr = qn->mutable_results();
    qn->set_identifier(-1);
    qc->set_tag("");
    qc->set_p_op(protobufs::And);
    qr->set_r_type(protobufs::List);
    string *key = qr->add_response_keys();
    *key = "Status";
    key = qr->add_response_keys();
    *key = "Since";
    qr->set_sort(true);
    qr->set_sort_key("Status");
    cmds.push_back(&cmdquery);
    query_count++;

    // No need to commit in this case. So just end TX
    protobufs::Command cmdtxend;
    // Commit here doesn't change anything. Just indicates end of TX
    cmdtxend.set_cmd_id(protobufs::Command::TxCommit);
    cmdtxend.set_tx_id(txid);
    cmds.push_back(&cmdtxend);
    query_count++;

    vector<vector<protobufs::CommandResponse *>> responses =
        qh.process_queries(cmds, query_count, true);
    int edgecount = 0, propcount = 0;
    for (int q = 0; q < query_count; ++q) {
      vector<protobufs::CommandResponse *> response = responses[q];
      for (auto it : response) {
        EXPECT_EQ(it->error_code(), protobufs::CommandResponse::Success)
            << it->error_msg();
        if (it->r_type() == protobufs::List) {
          auto mymap = it->prop_values();
          for (auto m_it : mymap) {
            // Assuming string for now
            protobufs::PropertyList &p = m_it.second;
            edgecount = 0;
            for (int i = 0; i < p.values_size(); ++i) {
              if (m_it.first == "Status") {
                if (i <= 1)
                  EXPECT_EQ(p.values(i).string_value(), "Old Adult");
                else
                  EXPECT_EQ(p.values(i).string_value(), "Young Adult");
              }
              print_property(m_it.first, p.values(i));
              edgecount++;
            }
            propcount++;
          }
        }
        // printf("\n");
      }
    }
    EXPECT_EQ(edgecount, 3) << "Not enough edges found";
    EXPECT_EQ(propcount, 2) << "Not enough properties read";
  }
  VDMSConfig::destroy();
  PMGDQueryHandler::destroy();
}

TEST(PMGDQueryHandler, queryNodeEdgeTestList) {
  VDMSConfig::init("unit_tests/config-pmgd-tests.json");
  PMGDQueryHandler::init();
  PMGDQueryHandler qh;

  vector<protobufs::Command *> cmds;

  {
    int txid = 1, query_count = 0;
    protobufs::Command cmdtx;
    cmdtx.set_cmd_id(protobufs::Command::TxBegin);
    cmdtx.set_tx_id(txid);
    cmds.push_back(&cmdtx);
    query_count++;

    // Constrain the starting nodes for the edge we want to access
    protobufs::Command cmdstartquery;
    cmdstartquery.set_cmd_id(protobufs::Command::QueryNode);
    cmdstartquery.set_tx_id(txid);
    protobufs::QueryNode *qn = cmdstartquery.mutable_query_node();
    protobufs::Constraints *qc = qn->mutable_constraints();
    protobufs::ResultInfo *qr = qn->mutable_results();
    qn->set_identifier(1);
    qc->set_tag("Patient");
    qc->set_p_op(protobufs::And);
    protobufs::PropertyPredicate *pp = qc->add_predicates();
    pp->set_key("Email");
    pp->set_op(protobufs::PropertyPredicate::Eq);
    protobufs::Property *p = pp->mutable_v1();
    p->set_type(protobufs::Property::StringType);
    // I think the key is not required here.
    p->set_key("Email");
    p->set_string_value("john.doe@abc.com");
    cmds.push_back(&cmdstartquery);
    query_count++;

    protobufs::Command cmdquery;
    cmdquery.set_cmd_id(protobufs::Command::QueryEdge);
    cmdquery.set_tx_id(txid);
    protobufs::QueryEdge *qe = cmdquery.mutable_query_edge();
    qc = qe->mutable_constraints();
    qr = qe->mutable_results();
    qe->set_identifier(-1);
    qe->set_src_node_id(1);
    qe->set_dest_node_id(-1);
    qc->set_tag("");
    qc->set_p_op(protobufs::And);
    pp = qc->add_predicates();
    pp->set_key("Status");
    pp->set_op(protobufs::PropertyPredicate::Eq);
    p = pp->mutable_v1();
    p->set_type(protobufs::Property::StringType);
    // I think the key is not required here.
    p->set_key("Status");
    p->set_string_value("Old Adult");
    qr->set_r_type(protobufs::List);
    string *key = qr->add_response_keys();
    *key = "Status";
    cmds.push_back(&cmdquery);
    query_count++;

    // No need to commit in this case. So just end TX
    protobufs::Command cmdtxend;
    // Commit here doesn't change anything. Just indicates end of TX
    cmdtxend.set_cmd_id(protobufs::Command::TxCommit);
    cmdtxend.set_tx_id(txid);
    cmds.push_back(&cmdtxend);
    query_count++;

    vector<vector<protobufs::CommandResponse *>> responses =
        qh.process_queries(cmds, query_count, true);
    int edgecount = 0, propcount = 0;
    for (int q = 0; q < query_count; ++q) {
      vector<protobufs::CommandResponse *> response = responses[q];
      for (auto it : response) {
        EXPECT_EQ(it->error_code(), protobufs::CommandResponse::Success)
            << it->error_msg();
        if (it->r_type() == protobufs::List) {
          auto mymap = it->prop_values();
          for (auto m_it : mymap) {
            // Assuming string for now
            protobufs::PropertyList &p = m_it.second;
            edgecount = 0;
            for (int i = 0; i < p.values_size(); ++i) {
              print_property(m_it.first, p.values(i));
              edgecount++;
            }
            propcount++;
          }
        }
        // printf("\n");
      }
    }
    EXPECT_EQ(edgecount, 1) << "Not enough edges found";
    EXPECT_EQ(propcount, 1) << "Not enough properties read";
  }
  VDMSConfig::destroy();
  PMGDQueryHandler::destroy();
}

TEST(PMGDQueryHandler, queryNodeEdgeDestTestList) {
  VDMSConfig::init("unit_tests/config-pmgd-tests.json");
  PMGDQueryHandler::init();
  PMGDQueryHandler qh;

  vector<protobufs::Command *> cmds;

  {
    int txid = 1, query_count = 0;
    protobufs::Command cmdtx;
    cmdtx.set_cmd_id(protobufs::Command::TxBegin);
    cmdtx.set_tx_id(txid);
    cmds.push_back(&cmdtx);
    query_count++;

    // Constrain the starting nodes for the edge we want to access
    protobufs::Command cmdstartquery;
    cmdstartquery.set_cmd_id(protobufs::Command::QueryNode);
    cmdstartquery.set_tx_id(txid);
    protobufs::QueryNode *qn = cmdstartquery.mutable_query_node();
    protobufs::Constraints *qc = qn->mutable_constraints();
    protobufs::ResultInfo *qr = qn->mutable_results();
    qn->set_identifier(1);
    qc->set_tag("Patient");
    qc->set_p_op(protobufs::And);
    protobufs::PropertyPredicate *pp = qc->add_predicates();
    pp->set_key("Email");
    pp->set_op(protobufs::PropertyPredicate::Eq);
    protobufs::Property *p = pp->mutable_v1();
    p->set_type(protobufs::Property::StringType);
    // I think the key is not required here.
    p->set_key("Email");
    p->set_string_value("john.doe@abc.com");
    cmds.push_back(&cmdstartquery);
    query_count++;

    protobufs::Command cmdadd;
    cmdadd.set_tx_id(txid);
    add_patient(cmdadd, 2, "Jane Foster", 70, "Tue Oct 1 13:59:24 PDT 1946",
                "jane.foster@pqr.com", FEMALE);
    cmds.push_back(&cmdadd);
    query_count++;

    protobufs::Command cmdedge;
    cmdedge.set_tx_id(txid);
    cmdedge.set_cmd_id(protobufs::Command::AddEdge);
    protobufs::AddEdge *ae = cmdedge.mutable_add_edge();
    ae->set_identifier(-1);
    protobufs::Edge *e = ae->mutable_edge();
    e->set_src(1);
    e->set_dst(2);
    e->set_tag("Friend");
    p = e->add_properties();
    p->set_type(protobufs::Property::TimeType);
    p->set_key("Since");
    p->set_time_value("Sat Sep 1 19:59:24 PDT 1956");
    p = e->add_properties();
    p->set_type(protobufs::Property::StringType);
    p->set_key("Status");
    p->set_string_value("Old Adult");
    cmds.push_back(&cmdedge);
    query_count++;

    protobufs::Command cmdquery;
    cmdquery.set_cmd_id(protobufs::Command::QueryEdge);
    cmdquery.set_tx_id(txid);
    protobufs::QueryEdge *qe = cmdquery.mutable_query_edge();
    qc = qe->mutable_constraints();
    qr = qe->mutable_results();
    qe->set_identifier(-1);
    qe->set_src_node_id(1);
    qe->set_dest_node_id(2);
    qc->set_tag("");
    qc->set_p_op(protobufs::And);
    pp = qc->add_predicates();
    pp->set_key("Status");
    pp->set_op(protobufs::PropertyPredicate::Eq);
    p = pp->mutable_v1();
    p->set_type(protobufs::Property::StringType);
    // I think the key is not required here.
    p->set_key("Status");
    p->set_string_value("Old Adult");
    qr->set_r_type(protobufs::List);
    string *key = qr->add_response_keys();
    *key = "Status";
    cmds.push_back(&cmdquery);
    query_count++;

    // No need to commit in this case. So just end TX
    protobufs::Command cmdtxend;
    // Commit here doesn't change anything. Just indicates end of TX
    cmdtxend.set_cmd_id(protobufs::Command::TxCommit);
    cmdtxend.set_tx_id(txid);
    cmds.push_back(&cmdtxend);
    query_count++;

    vector<vector<protobufs::CommandResponse *>> responses =
        qh.process_queries(cmds, query_count, false);
    int edgecount = 0, propcount = 0;
    for (int q = 0; q < query_count; ++q) {
      vector<protobufs::CommandResponse *> response = responses[q];
      for (auto it : response) {
        EXPECT_EQ(it->error_code(), protobufs::CommandResponse::Success)
            << it->error_msg();
        if (it->r_type() == protobufs::List) {
          auto mymap = it->prop_values();
          for (auto m_it : mymap) {
            // Assuming string for now
            protobufs::PropertyList &p = m_it.second;
            edgecount = 0;
            for (int i = 0; i < p.values_size(); ++i) {
              print_property(m_it.first, p.values(i));
              edgecount++;
            }
            propcount++;
          }
        }
        // printf("\n");
      }
    }
    EXPECT_EQ(edgecount, 1) << "Not enough edges found";
    EXPECT_EQ(propcount, 1) << "Not enough properties read";
  }
  VDMSConfig::destroy();
  PMGDQueryHandler::destroy();
}

TEST(PMGDQueryHandler, queryUpdateEdge) {
  VDMSConfig::init("unit_tests/config-pmgd-tests.json");
  PMGDQueryHandler::init();
  PMGDQueryHandler qh;

  vector<protobufs::Command *> cmds;

  {
    int txid = 1, query_count = 0;
    protobufs::Command cmdtx;
    cmdtx.set_cmd_id(protobufs::Command::TxBegin);
    cmdtx.set_tx_id(txid);
    cmds.push_back(&cmdtx);
    query_count++;

    // Constrain the starting nodes for the edge we want to access
    protobufs::Command cmdstartquery;
    cmdstartquery.set_cmd_id(protobufs::Command::QueryNode);
    cmdstartquery.set_tx_id(txid);
    protobufs::QueryNode *qn = cmdstartquery.mutable_query_node();
    protobufs::Constraints *qc = qn->mutable_constraints();
    protobufs::ResultInfo *qr = qn->mutable_results();
    qn->set_identifier(1);
    qc->set_tag("Patient");
    qc->set_p_op(protobufs::And);
    protobufs::PropertyPredicate *pp = qc->add_predicates();
    pp->set_key("Email");
    pp->set_op(protobufs::PropertyPredicate::Eq);
    protobufs::Property *p = pp->mutable_v1();
    p->set_type(protobufs::Property::StringType);
    // I think the key is not required here.
    p->set_key("Email");
    p->set_string_value("john.doe@abc.com");
    cmds.push_back(&cmdstartquery);
    query_count++;

    protobufs::Command cmdadd;
    cmdadd.set_tx_id(txid);
    add_patient(cmdadd, 2, "Jane Foster", 70, "Tue Oct 1 13:59:24 PDT 1946",
                "jane.foster@pqr.com", FEMALE);
    cmds.push_back(&cmdadd);
    query_count++;

    protobufs::Command cmdedge;
    cmdedge.set_tx_id(txid);
    cmdedge.set_cmd_id(protobufs::Command::AddEdge);
    protobufs::AddEdge *ae = cmdedge.mutable_add_edge();
    ae->set_identifier(-1);
    protobufs::Edge *e = ae->mutable_edge();
    e->set_src(1);
    e->set_dst(2);
    e->set_tag("Friend");
    p = e->add_properties();
    p->set_type(protobufs::Property::TimeType);
    p->set_key("Since");
    p->set_time_value("Sat Sep 1 19:59:24 PDT 1956");
    p = e->add_properties();
    p->set_type(protobufs::Property::StringType);
    p->set_key("Status");
    p->set_string_value("Old Adult");
    cmds.push_back(&cmdedge);
    query_count++;

    protobufs::Command cmdquery;
    cmdquery.set_cmd_id(protobufs::Command::QueryEdge);
    cmdquery.set_tx_id(txid);
    protobufs::QueryEdge *qe = cmdquery.mutable_query_edge();
    qc = qe->mutable_constraints();
    qr = qe->mutable_results();
    qe->set_identifier(10);
    qe->set_src_node_id(1);
    qe->set_dest_node_id(2);
    qc->set_tag("");
    qc->set_p_op(protobufs::And);
    pp = qc->add_predicates();
    pp->set_key("Status");
    pp->set_op(protobufs::PropertyPredicate::Eq);
    p = pp->mutable_v1();
    p->set_type(protobufs::Property::StringType);
    // I think the key is not required here.
    p->set_key("Status");
    p->set_string_value("Old Adult");
    qr->set_r_type(protobufs::List);
    string *key = qr->add_response_keys();
    *key = "Status";
    cmds.push_back(&cmdquery);
    query_count++;

    protobufs::Command cmdupdate;
    cmdupdate.set_cmd_id(protobufs::Command::UpdateEdge);
    cmdupdate.set_tx_id(txid);
    protobufs::UpdateEdge *ue = cmdupdate.mutable_update_edge();

    // The identifier here will be the identifier used for search
    // since we are going to update properties of the edge found
    // in the previous search
    ue->set_identifier(10);
    p = ue->add_properties();
    p->set_type(protobufs::Property::StringType);
    p->set_key("StartHospital");
    p->set_string_value("Kaiser1");
    p = ue->add_properties();
    p->set_type(protobufs::Property::StringType);
    p->set_key("Status");
    p->set_string_value("Medium Adult");

    // Remove the extra properties
    ue->add_remove_props("Since");
    cmds.push_back(&cmdupdate);

    // Re-query with different properties
    protobufs::Command cmdqueryu;
    cmdqueryu.set_cmd_id(protobufs::Command::QueryEdge);
    cmdqueryu.set_tx_id(txid);
    qe = cmdqueryu.mutable_query_edge();
    qc = qe->mutable_constraints();
    qr = qe->mutable_results();
    qe->set_identifier(-1);
    qc->set_tag("");
    qc->set_p_op(protobufs::And);
    pp = qc->add_predicates();
    pp->set_key("Status");
    pp->set_op(protobufs::PropertyPredicate::Eq);
    p = pp->mutable_v1();
    p->set_type(protobufs::Property::StringType);
    // I think the key is not required here.
    p->set_key("Status");
    p->set_string_value("Medium Adult");
    qr->set_r_type(protobufs::List);
    key = qr->add_response_keys();
    *key = "Since";
    key = qr->add_response_keys();
    *key = "StartHospital";
    cmds.push_back(&cmdqueryu);
    query_count++;

    protobufs::Command cmdtxend;
    // Commit here doesn't change anything. Just indicates end of TX
    cmdtxend.set_cmd_id(protobufs::Command::TxCommit);
    cmdtxend.set_tx_id(txid);
    cmds.push_back(&cmdtxend);
    query_count++;

    vector<vector<protobufs::CommandResponse *>> responses =
        qh.process_queries(cmds, query_count, false);
    int edgecount = 0, propcount = 0;
    for (int q = 0; q < query_count; ++q) {
      vector<protobufs::CommandResponse *> response = responses[q];
      int qcount = 0;
      for (auto it : response) {
        EXPECT_EQ(it->error_code(), protobufs::CommandResponse::Success)
            << it->error_msg();
        if (it->r_type() == protobufs::List) {
          if (qcount == 4) { // First query
            auto mymap = it->prop_values();
            for (auto m_it : mymap) {
              // Assuming string for now
              protobufs::PropertyList &p = m_it.second;
              edgecount = 0;
              for (int i = 0; i < p.values_size(); ++i) {
                print_property(m_it.first, p.values(i));
                edgecount++;
              }
              propcount++;
            }
            EXPECT_EQ(propcount, 1) << "Not enough properties read";
            propcount = 0;
          } else if (q == 6) {
            auto mymap = it->prop_values();
            for (auto m_it : mymap) {
              // Assuming string for now
              protobufs::PropertyList &p = m_it.second;
              edgecount = 0;
              for (int i = 0; i < p.values_size(); ++i) {
                print_property(m_it.first, p.values(i));
                edgecount++;
              }
              propcount++;
            }
            EXPECT_EQ(propcount, 2) << "Not enough properties read";
            propcount = 0;
          }
        }
        if (it->r_type() == protobufs::Count) {
          EXPECT_EQ(it->op_int_value(), 1)
              << "Doesn't match expected update count";
        }
        qcount++;
        // printf("\n");
      }
    }
    EXPECT_EQ(edgecount, 1) << "Not enough edges found";
  }
  VDMSConfig::destroy();
  PMGDQueryHandler::destroy();
}
