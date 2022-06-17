#include "meta_data_helper.h"

TEST(CLIENT_CPP, add_BB){
     Meta_Data* meta_obj=new Meta_Data();
     meta_obj->_aclient.reset ( new VDMS::VDMSClient(meta_obj->get_server(), meta_obj->get_port()));
    Json::Value tuple ;
    tuple=meta_obj->constuct_BB(false);
    VDMS::Response response =meta_obj->_aclient->query(meta_obj->_fastwriter.write(tuple));
    Json::Value result;
    meta_obj->_reader.parse(response.json.c_str(),  result);

    int status1 = result[0]["AddBoundingBox"]["status"].asInt();
    EXPECT_EQ(status1, 0);
}

TEST(CLIENT_CPP, add_BB_with_image){
    std::fstream jpgimage("../tests/images/large1.jpg");
    jpgimage.seekg(0, jpgimage.end);
    int length = jpgimage.tellg();
    // std::cout<<"Length " <<length<<std::endl;
    jpgimage.seekg(0, jpgimage.beg);

    char* buffer = new char[length];
    jpgimage.read(buffer, length);
    jpgimage.close();
    std::vector<std::string*> blobs;
      

    std::string *bytes_str = new std::string(buffer);
    
    blobs.push_back(bytes_str);
       
    Meta_Data* meta_obj=new Meta_Data();
     meta_obj->_aclient.reset ( new VDMS::VDMSClient(meta_obj->get_server(), meta_obj->get_port()));
    Json::Value tuple ;
    tuple=meta_obj->constuct_BB(true);
    // std::cout<<tuple<<std::endl;
    VDMS::Response response =meta_obj->_aclient->query(meta_obj->_fastwriter.write(tuple), blobs);
    Json::Value result;
    meta_obj->_reader.parse(response.json.c_str(),  result);

    int status1 = result[0]["AddBoundingBox"]["status"].asInt();
    EXPECT_EQ(status1, 0);
}