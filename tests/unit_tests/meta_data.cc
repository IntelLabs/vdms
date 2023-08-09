#include "meta_data_helper.h"

Meta_Data::Meta_Data() {}

Json::Value Meta_Data::construct_Flinng_Set(std::string &name, int &dim) {

  Json::Value descriptor_set;
  Json::Value set_query;
  Json::Value tuple;
  descriptor_set["name"] = name;
  descriptor_set["dimensions"] = dim;
  descriptor_set["metric"] = "IP";
  descriptor_set["engine"] = "Flinng";
  descriptor_set["flinng_num_rows"] = 3;
  descriptor_set["flinng_cells_per_row"] = 100;
  descriptor_set["flinng_num_hash_tables"] = 12;
  descriptor_set["flinng_hashes_per_table"] = 10;
  descriptor_set["flinng_sub_hash_bits"] = 2;
  descriptor_set["flinng_cut_off"] = 6;
  set_query["AddDescriptorSet"] = descriptor_set;

  return set_query;
}

Json::Value Meta_Data::construct_flinng_descriptor() {
  Json::Value tuple;
  std::shared_ptr<VDMS::VDMSClient> test_aclient;
  std::string name = "flinng_test_2060";
  int dim = 100;
  tuple.append(construct_Flinng_Set(name, dim));
  test_aclient.reset(new VDMS::VDMSClient(get_server(), get_port()));
  VDMS::Response response = test_aclient->query(_fastwriter.write(tuple));
  Json::Value result;
  _reader.parse(response.json.c_str(), result);
  Json::Value AddDesc;
  Json::Value Desc;

  Desc["set"] = "flinng_test_2060";
  Desc["label"] = "Person";
  Desc["_ref"] = 1;
  Desc["properties"]["id"] = 123;
  Desc["properties"]["name"] = "Ali";
  AddDesc["AddDescriptor"] = Desc;
  tuple.append(AddDesc);
  return tuple;
}

Json::Value Meta_Data::construct_descriptor() {
  Json::Value descriptor_set;
  Json::Value set_query;
  Json::Value tuple;
  std::shared_ptr<VDMS::VDMSClient> test_aclient;
  descriptor_set["name"] = "features_vectors_store1";
  descriptor_set["dimensions"] = 1000;
  set_query["AddDescriptorSet"] = descriptor_set;
  tuple.append(set_query);
  test_aclient.reset(new VDMS::VDMSClient(get_server(), get_port()));
  VDMS::Response response = test_aclient->query(_fastwriter.write(tuple));
  Json::Value result;
  _reader.parse(response.json.c_str(), result);
  Json::Value AddDesc;
  Json::Value Desc;

  Desc["set"] = "features_vectors_store1";
  Desc["label"] = "Person";
  Desc["_ref"] = 1;
  Desc["properties"]["id"] = 123;
  Desc["properties"]["name"] = "Ali";
  AddDesc["AddDescriptor"] = Desc;
  tuple.append(AddDesc);
  return tuple;
}

Json::Value Meta_Data::construct_find_descriptor() {
  Json::Value FindDesc;
  Json::Value Desc;
  Json::Value tuple;
  //   Desc["results"]["count"] = "";
  // Desc["constraints"]["id"][0] =">=";
  // Desc["constraints"]["id"][1] =100;
  Desc["results"]["list"][0] = "_distance";
  Desc["results"]["list"][1] = "id";
  Desc["set"] = "features_vectors_store1";
  Desc["k_neighbors"] = 5;
  //   Desc["blob"] =true;
  FindDesc["FindDescriptor"] = Desc;
  tuple.append(FindDesc);
  FindDesc.clear();
  Desc.clear();
  return tuple;
}

Json::Value Meta_Data::construct_find_flinng_descriptor() {
  Json::Value FindDesc;
  Json::Value Desc;
  Json::Value tuple;
  Desc["results"]["list"][0] = "_distance";
  Desc["results"]["list"][1] = "id";
  Desc["set"] = "flinng_test_2060";
  Desc["k_neighbors"] = 5;
  //   Desc["blob"] =true;
  FindDesc["FindDescriptor"] = Desc;
  tuple.append(FindDesc);
  FindDesc.clear();
  Desc.clear();
  return tuple;
}

Json::Value Meta_Data::constuct_image(bool add_operation,
                                      Json::Value operations) {

  Json::Value image;
  Json::Value add_image;
  Json::Value tuple;
  image["properties"]["Name"] = "sample-image";
  image["properties"]["ID"] = 1;
  image["format"] = "png";
  image["_ref"] = 12;
  if (add_operation) {
    image["operations"] = operations;
  }
  add_image["AddImage"] = image;
  tuple.append(add_image);
  return tuple;
}

Json::Value Meta_Data::constuct_video(bool add_operation) {

  Json::Value video;
  Json::Value add_video;
  Json::Value tuple;
  video["properties"]["Name"] = "sample-video";
  video["properties"]["ID"] = 1;
  video["container"] = "avi";
  video["codec"] = "xvid";
  // video["_ref"]=1209;
  // if( add_operation)
  // {
  //     video["operations"]=operations;
  // }
  add_video["AddVideo"] = video;
  tuple.append(add_video);
  return tuple;
}

Json::Value Meta_Data::construct_find_image() {
  Json::Value tuple;

  Json::Value cons;
  cons["Name"][0] = "==";
  cons["Name"][1] = "sample-image";

  Json::Value results;
  results["blob"] = false;
  results["list"][0] = "Name";
  results["list"][1] = "ID";

  Json::Value image;
  image["_ref"] = 1;
  image["constraints"] = cons;
  image["results"] = results;

  Json::Value find_image;
  find_image["FindImage"] = image;

  tuple.append(find_image);
  return tuple;
}

Json::Value Meta_Data::construct_find_image_withop(Json::Value operations) {
  Json::Value tuple;

  Json::Value results;
  results["blob"] = true;

  Json::Value image;
  image["results"] = results;
  image["operations"] = operations;

  Json::Value find_image;
  find_image["FindImage"] = image;

  tuple.append(find_image);
  return tuple;
}

std::string *Meta_Data::read_blob(std::string &fname) {
  std::string video;
  std::ifstream video_file(fname,
                           std::ios::in | std::ios::binary | std::ios::ate);

  video.resize(video_file.tellg());

  video_file.seekg(0, std::ios::beg);
  if (!video_file.read(&video[0], video.size()))
    std::cout << "error" << std::endl;
  std::string *bytes_str = new std::string(video);
  // std::cout << *bytes_str <<std::endl;
  return bytes_str;
}

Json::Value Meta_Data::construct_updateBlob() {
  Json::Value blob;
  Json::Value update_blob;
  Json::Value tuple;
  Json::Value cons;
  Json::Value results;
  cons["Name"][0] = "==";
  cons["Name"][1] = "sample1-Blob-new";
  blob["constraints"] = cons;
  // blob["_ref"]=12;
  blob["properties"]["colored"] = "false";
  blob["properties"]["length"] = 200;

  update_blob["UpdateBlob"] = blob;

  tuple.append(update_blob);
  return tuple;
}

Json::Value Meta_Data::construct_findBlob() {
  Json::Value tuple;

  Json::Value cons;
  cons["Name"][0] = "==";
  cons["Name"][1] = "sample1-Blob-new";

  Json::Value results;
  results["list"][0] = "Name";

  Json::Value blob;
  blob["_ref"] = 12;
  blob["constraints"] = cons;
  blob["results"] = results;

  Json::Value find_blob;
  find_blob["FindBlob"] = blob;

  tuple.append(find_blob);
  return tuple;
}

Json::Value Meta_Data::construct_Blob() {
  Json::Value blob;
  Json::Value add_blob;
  Json::Value tuple;
  blob["properties"]["Name"] = "sample1-Blob-new";
  blob["properties"]["colored"] = "true";
  blob["properties"]["file"] = "audio";
  blob["_ref"] = 12;
  add_blob["AddBlob"] = blob;

  tuple.append(add_blob);

  return tuple;
}

Json::Value Meta_Data::constuct_BB(bool with_image) {
  Json::Value image;
  Json::Value add_image;
  Json::Value tuple;
  image["properties"]["Name"] = "sample1-BB";
  image["format"] = "png";
  image["_ref"] = 12;
  add_image["AddImage"] = image;

  Json::Value BB_coord;
  Json::Value BB;
  Json::Value query;
  BB_coord["x"] = 10;
  BB_coord["y"] = 10;
  BB_coord["h"] = 100;
  BB_coord["w"] = 100;
  BB["rectangle"] = BB_coord;
  if (with_image) {
    BB["image"] = 12;
    tuple.append(add_image);
  }
  query["AddBoundingBox"] = BB;

  tuple.append(query);
  return tuple;
}

Json::Value Meta_Data::construct_add_query(int ref, bool const_on,
                                           bool experiation) {
  Json::Value entity;
  Json::Value query;
  Json::Value tuple;
  entity["class"] = "Store";
  entity["_ref"] = ref;
  entity["properties"]["Name"] = "Walmart";
  entity["properties"]["Type"] = "grocerys";
  entity["properties"]["ID"] = 1;
  entity["properties"]["Address"] = "1428 alex way, Hillsboro 97124";
  if (experiation) {
    entity["properties"]["_expiration"] = 10;
  }
  if (const_on) {
    entity["constraints"]["Name"][0] = "==";
    entity["constraints"]["Name"][1] = "Walmart";
    entity["constraints"]["ID"][0] = "==";
    entity["constraints"]["ID"][1] = 2;
  }
  query["AddEntity"] = entity;

  return query;
}

Json::Value Meta_Data::construct_add_area(int ref, bool const_on) {
  Json::Value entity;
  Json::Value query;
  Json::Value tuple;
  entity["class"] = "Store";
  entity["_ref"] = ref;
  entity["properties"]["Name"] = "Area1";
  entity["properties"]["Type"] = "Sport";
  entity["properties"]["ID"] = 1;

  query["AddEntity"] = entity;

  if (const_on) {
    query["AddEntity"]["constraints"]["Name"][0] = "==";
    query["AddEntity"]["constraints"]["Name"][1] = "Sport";
    query["AddEntity"]["constraints"]["ID"][0] = "==";
    query["AddEntity"]["constraints"]["ID"][1] = 1;
  }
  return query;
}

Json::Value Meta_Data::construct_add_connection(int ref1, int ref2,
                                                bool const_on) {
  Json::Value connection;
  Json::Value starting_date;
  Json::Value query;
  Json::Value tuple;

  connection["class"] = "has";
  connection["ref1"] = ref1;
  connection["ref2"] = ref2;

  connection["properties"]["type"] = "grocerys";
  starting_date["_date"] = "Tue Apr 23 17:12:24 PDT 2018";
  connection["properties"]["date"] = starting_date;

  query["AddConnection"] = connection;

  if (const_on) {
    query["AddConnection"]["constraints"]["type"][0] = "==";
    query["AddConnection"]["constraints"]["Name"][1] = "grocerys";
  }
  return query;
}

Json::Value Meta_Data::construct_find_entity(bool auto_delete = false,
                                             bool expiration = false) {

  Json::Value entity;
  Json::Value query;
  Json::Value tuple;
  entity["class"] = "Store";
  // entity["_ref"] = 1;

  entity["constraints"]["Name"][0] = "==";
  entity["constraints"]["Name"][1] = "Walmart";
  if (auto_delete) {
    entity["constraints"]["_deletion"][0] = "==";
    entity["constraints"]["_deletion"][1] = 1;
  }
  if (expiration) {
    entity["constraints"]["_expiration"][0] = "<";
    entity["constraints"]["_expiration"][1] = 10;
  }

  Json::Value result;
  result["list"][0] = "ID";
  if (expiration) {
    result["list"][0] = "_expiration";
    result["list"][1] = "_creation";
  }
  entity["results"] = result;

  query["FindEntity"] = entity;
  return query;
}