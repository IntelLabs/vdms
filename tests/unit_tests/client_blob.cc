#include "meta_data_helper.h"
TEST(BLOB, add_Blob){
    std::string filename ="../tests/test_images/large1.jpg";
    std::vector<std::string*> blobs;

    Meta_Data* meta_obj=new Meta_Data();
    blobs.push_back(meta_obj->read_blob(filename));
     meta_obj->_aclient.reset ( new VDMS::VDMSClient(meta_obj->get_server(), meta_obj->get_port()));
    Json::Value tuple ;
    tuple=meta_obj->construct_Blob();


    VDMS::Response response =meta_obj->_aclient->query(meta_obj->_fastwriter.write(tuple), blobs);
    Json::Value result;

    meta_obj->_reader.parse(response.json.c_str(),  result);
    int status1 = result[0]["AddBlob"]["status"].asInt();
    EXPECT_EQ(status1, 0);
}

TEST(BLOB, update_Blob){

    Meta_Data* meta_obj=new Meta_Data();
    meta_obj->_aclient.reset ( new VDMS::VDMSClient(meta_obj->get_server(), meta_obj->get_port()));
    Json::Value tuple ;
    tuple=meta_obj->construct_updateBlob();
    VDMS::Response response =meta_obj->_aclient->query(meta_obj->_fastwriter.write(tuple));
    Json::Value result;

    meta_obj->_reader.parse(response.json.c_str(),  result);
    int status1 = result[0]["status"].asInt();

    EXPECT_EQ(status1, 0);
}
TEST(BLOB, find_Blob){

    Meta_Data* meta_obj=new Meta_Data();
    meta_obj->_aclient.reset ( new VDMS::VDMSClient(meta_obj->get_server(), meta_obj->get_port()));
    Json::Value tuple ;
    tuple=meta_obj->construct_findBlob();
    VDMS::Response response =meta_obj->_aclient->query(meta_obj->_fastwriter.write(tuple));
    Json::Value result;

    meta_obj->_reader.parse(response.json.c_str(),  result);
    int status1 = result[0]["status"].asInt();

    EXPECT_EQ(status1, 0);
}