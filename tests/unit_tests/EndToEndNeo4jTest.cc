#include <filesystem>

#include "OpsIOCoordinator.h"
#include "VDMSConfig.h"
#include "meta_data_helper.h"

const std::string TMP_DIRNAME = "tests_output_dir/";

Json::Value construct_cypher_add_img(std::string prop_name,
                                     std::string prop_value, std::string label,
                                     std::string tgt_fmt,
                                     Json::Value operations);
Json::Value construct_cypher_find_img(std::string prop_name,
                                      std::string prop_value, std::string label,
                                      std::string tgt_fmt,
                                      Json::Value operations);
Json::Value construct_cypher_add_md(std::string prop_name,
                                    std::string prop_value, std::string label);
Json::Value construct_cypher_find_md(std::string prop_name,
                                     std::string prop_value, std::string label);

Json::Value construct_cypher_add_img(std::string prop_name,
                                     std::string prop_value, std::string label,
                                     std::string tgt_fmt,
                                     Json::Value operations) {

  Json::Value image;
  Json::Value add_image;
  Json::Value tuple;
  std::string cypher_q;

  cypher_q = "CREATE (VDMSNODE:" + label + "{" + prop_name + ":" + "\"" +
             prop_value + "\"" + "})";
  image["cypher"] = cypher_q;
  image["target_data_type"] = "img";
  image["target_format"] = tgt_fmt;
  image["operations"] = operations;

  add_image["NeoAdd"] = image;
  tuple.append(add_image);

  return tuple;
}

Json::Value construct_cypher_find_img(std::string prop_name, std::string label,
                                      std::string tgt_fmt,
                                      Json::Value operations) {

  Json::Value image;
  Json::Value find_image;
  Json::Value tuple;
  std::string cypher_q;

  cypher_q = "MATCH (VDMSNODE:" + label +
             ") RETURN VDMSNODE.img_loc, VDMSNODE." + prop_name;
  image["cypher"] = cypher_q;
  image["target_data_type"] = "img";
  image["target_format"] = tgt_fmt;
  image["operations"] = operations;

  find_image["NeoFind"] = image;
  tuple.append(find_image);

  return tuple;
}

Json::Value construct_cypher_add_md(std::string prop_name,
                                    std::string prop_value, std::string label) {

  Json::Value md_q;
  Json::Value add_md;
  Json::Value tuple;
  std::string cypher_q;

  cypher_q = "CREATE (VDMSNODE:" + label + "{" + prop_name + ":" + "\"" +
             prop_value + "\"" + "})";
  md_q["cypher"] = cypher_q;
  md_q["target_data_type"] = "md_only";

  add_md["NeoAdd"] = md_q;
  tuple.append(add_md);

  return tuple;
}

Json::Value construct_cypher_find_md(std::string prop_name, std::string label) {

  Json::Value md_q;
  Json::Value add_md;
  Json::Value tuple;
  std::string cypher_q;

  cypher_q = "MATCH (VDMSNODE:" + label + ") RETURN VDMSNODE." + prop_name;
  md_q["cypher"] = cypher_q;
  md_q["target_data_type"] = "md_only";

  add_md["NeoAdd"] = md_q;
  tuple.append(add_md);

  return tuple;
}

class Neo4JE2ETest : public ::testing::Test {

protected:
  std::string vdms_server_;
  int vdms_port_;

  virtual void SetUp() {
    VDMS::VDMSConfig::init(TMP_DIRNAME + "config-neo4j-e2e.json");
    vdms_server_ = "localhost";
    vdms_port_ = 55559;
  }

  virtual void TearDown() { VDMS::VDMSConfig::destroy(); }

  void add_find_img_test() {

    try{

      std::string image;
      Meta_Data *meta_obj = new Meta_Data();
      VDMS::VDMSClient qclient(vdms_server_, vdms_port_);

      // Create operations block
      Json::Value op;
      op["type"] = "crop";
      op["width"] = 640;
      op["height"] = 480;
      op["x"] = 0;
      op["y"] = 0;

      Json::Value ops_tuple;
      ops_tuple.append(op);

      // Construct Query
      Json::Value tuple;

      tuple = construct_cypher_add_img("test_prop_name", "test_prop_value",
                                      "test_label", "jpg", ops_tuple);

      // get binary image blob
      std::vector<std::string *> blobs;
      std::string *bytes_str;
      std::string filename = "test_images/large1.jpg";

      ASSERT_TRUE(std::filesystem::exists(filename));

      bytes_str = meta_obj->read_blob(filename);
      blobs.push_back(bytes_str);

      VDMS::Response response =
          qclient.query(meta_obj->_fastwriter.write(tuple), blobs);
      Json::Value result;

      meta_obj->_reader.parse(response.json.c_str(), result);

      // ON to the retrieval!
      Json::Value find_op;
      find_op["type"] = "resize";
      find_op["width"] = 100;
      find_op["height"] = 100;
      Json::Value find_ops_tuple;
      find_ops_tuple.append(find_op);


      Json::Value find_tuple;
      find_tuple = construct_cypher_find_img("test_prop_name", "test_label",
                                            "jpg", find_ops_tuple);
      response = qclient.query(meta_obj->_fastwriter.write(find_tuple));

      meta_obj->_reader.parse(response.json.c_str(), result);

      Json::Value metadata_res;
      metadata_res = result[0]["metadata_res"];
      std::string prop_res =
          metadata_res[0]["VDMSNODE.test_prop_name"].asString();

      delete meta_obj;

      // verifying metadata response and expected image/blob size response
      ASSERT_STREQ(prop_res.c_str(), "test_prop_value");
      ASSERT_EQ(8118, response.blobs[0].size());
    } catch (VCL::Exception e) {
      print_exception(e);
      FAIL() << "VCL::Exception in add_find_img_test()" << std::endl;
    } catch (std::exception &e) {
        std::string error_message = std::string("Exception in add_find_img_test(): ") + e.what();
        std::cerr << error_message << std::endl;
        FAIL() << error_message;
    }
  }

  void add_find_md_test() {

    try{
      Meta_Data *meta_obj = new Meta_Data();
      VDMS::VDMSClient qclient(vdms_server_, vdms_port_);
      VDMS::Response response;
      Json::Value result;
      Json::Value tuple;
      std::string md_res_1;
      std::string md_res_2;


      // Construct 2 add queries
      tuple = construct_cypher_add_md("test_md_name", "test_md_value_1",
                                      "md_only_label");
      response = qclient.query(meta_obj->_fastwriter.write(tuple));
      //TODO Delete this line
      std::cerr << "Response 1:" << response.json.c_str() << std::endl;


      tuple = construct_cypher_add_md("test_md_name", "test_md_value_2",
                                      "md_only_label");
      response = qclient.query(meta_obj->_fastwriter.write(tuple));


      tuple = construct_cypher_find_md("test_md_name", "md_only_label");
      response = qclient.query(meta_obj->_fastwriter.write(tuple));

      meta_obj->_reader.parse(response.json.c_str(), result);
      md_res_1 = result[0]["metadata_res"][0]["VDMSNODE.test_md_name"].asString();
      md_res_2 = result[0]["metadata_res"][1]["VDMSNODE.test_md_name"].asString();
      delete meta_obj;

      ASSERT_STREQ(md_res_1.c_str(), "test_md_value_1");
      ASSERT_STREQ(md_res_2.c_str(), "test_md_value_2");
    } catch (VCL::Exception e) {
      print_exception(e);
      FAIL() << "VCL::Exception in add_find_md_test()" << std::endl;
    } catch (std::exception &e) {
        std::string error_message = std::string("Exception in add_find_md_test(): ") + e.what();
        std::cerr << error_message << std::endl;
        FAIL() << error_message;
    }
  }

}; // end test class

TEST_F(Neo4JE2ETest, E2E_Neo4j_Add_Find_Img) { add_find_img_test(); }
TEST_F(Neo4JE2ETest, E2E_Neo4j_Add_Find_Metadata) { add_find_md_test(); }
