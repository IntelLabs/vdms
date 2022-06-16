#include "meta_data_helper.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

using std::cout; using std::cerr;
using std::endl; using std::string;
using std::ifstream; using std::ostringstream;

string readFileIntoString(const string& path) {
    auto ss = ostringstream{};
    ifstream input_file(path);
    if (!input_file.is_open()) {
        cerr << "Could not open the file - '"
             << path << "'" << endl;
        exit(EXIT_FAILURE);
    }
    ss << input_file.rdbuf();
    return ss.str();
}



TEST(CLIENT_CPP, add_single_video){
    

    // std::string video;
    std::stringstream video;
    std::vector<std::string*> blobs;

   
    const char  *_video_id ="../tests/videos/Megamind.avi";
     std::ifstream ifile;
     ifile.open(_video_id);

    int fsize;
    char* inBuf;
    ifile.seekg(0, std::ios::end);
    fsize = (long)ifile.tellg();
    ifile.seekg(0, std::ios::beg);
    inBuf = new char[fsize];
    ifile.read(inBuf, fsize);
    std::string blob =  (std::string(inBuf));
    ifile.close();
    delete[] inBuf;
       
    
    std::string* bytes_str =new std::string(blob);
    blobs.push_back(bytes_str);
     Meta_Data* meta_obj=new Meta_Data();
     meta_obj->_aclient.reset ( new VDMS::VDMSClient(meta_obj->get_server(), meta_obj->get_port()));
    Json::Value tuple ;
    tuple=meta_obj->constuct_video(false);
  
    
    std::cout<< "Printing bytes_str " <<  bytes_str  << "\t"<< blobs[0] << std::endl;
    
    VDMS::Response response =meta_obj->_aclient->query(meta_obj->_fastwriter.write(tuple), blobs);
    Json::Value result;
    meta_obj->_reader.parse(response.json.c_str(),  result);

    int status1 = result[0]["AddVideo"]["status"].asInt();
    EXPECT_EQ(status1, 0);


}
