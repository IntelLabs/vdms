#include "QueryHandler.h"
#include <string>
#include <fstream>
#include <iostream>
#include "gtest/gtest.h"

#include <mutex>
#include <vector>
#include "protobuf/pmgdMessages.pb.h" // Protobuff implementation
#include "jarvis.h"

using namespace athena;
using namespace Jarvis;
//using namespace pmgd;
using namespace std;

TEST(QueryHandler, addTest){
  //cout << "Testing Query handler add protobufs\n";
  std::ifstream ifile;
  int fsize;
  char * inBuf;
   std::cout<<"FILE OPEND"<<std::endl;
  ifile.open("query_sample.json", std::ifstream::in);
  std::cout<<"FILE OPEND"<<std::endl;
  ifile.seekg(0, std::ios::end);
  fsize = (int)ifile.tellg();
  ifile.seekg(0, std::ios::beg);
  inBuf = new char[fsize];
  ifile.read(inBuf, fsize);
  std::string json_query = std::string(inBuf);
  ifile.close();
  delete[] inBuf;
  std::cout<<json_query<<std::endl;

  Graph db("jsongraph", Graph::Create);

  // Since PMGD is still single threaded, provide a lock for the DB
  mutex dblock;

  QueryHandler query_handler(&db, &dblock);

  query_handler.process_query(json_query );

  //EXPECT_EQ(nodecount, 2) << "Not enough nodes found";
  //EXPECT_EQ(propcount, 2) << "Not enough properties read";
}
