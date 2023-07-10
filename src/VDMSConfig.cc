/**
 * @file   VDMSConfig.cc
 *
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

#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <jsoncpp/json/json.h>

#include "VDMSConfig.h"

#define DEFAULT_PATH_ROOT "db"
#define DEFAULT_PATH_PMGD "graph"
#define DEFAULT_PATH_IMAGES "images"
#define DEFAULT_PATH_JPG "jpg"
#define DEFAULT_PATH_PNG "png"
#define DEFAULT_PATH_TDB "tdb"
#define DEFAULT_PATH_BIN "bin"
#define DEFAULT_PATH_BLOBS "blobs"
#define DEFAULT_PATH_VIDEOS "videos"
#define DEFAULT_PATH_DESCRIPTORS "descriptors"
#define DEFAULT_PATH_TMP "tmp"
#define DEFAULT_STORAGE_TYPE "local"
#define DEFAULT_BUCKET_NAME "vdms_bucket"

using namespace VDMS;

VDMSConfig *VDMSConfig::cfg;

bool VDMSConfig::init(std::string config_file) {
  if (cfg)
    return false;

  cfg = new VDMSConfig(config_file);
  return true;
}

void VDMSConfig::destroy() {
  if (cfg) {
    delete cfg;
    cfg = NULL;
  }
}

VDMSConfig *VDMSConfig::instance() {
  if (cfg)
    return cfg;

  std::cout << "ERROR: Config not init" << std::endl;
  return NULL;
}

VDMSConfig::VDMSConfig(std::string config_file) {
  Json::Reader reader;
  std::ifstream file(config_file);

  bool parsingSuccessful = reader.parse(file, json_config);

  if (!parsingSuccessful) {
    std::cout << "Error parsing config file." << std::endl;
    std::cout << "Exiting..." << std::endl;
    exit(0);
  }

  build_dirs();
}

int VDMSConfig::get_int_value(std::string val, int def) {
  return json_config.get(val, def).asInt();
}

std::string VDMSConfig::get_string_value(std::string val, std::string def) {
  return json_config.get(val, def).asString();
}

// This is a function that createa a directory structure with DIRECTORY_LAYERS
// levels with each layer with DIRECTORIES_PER_LAYER ^ n directories. This
// function is recursive so will call itself to expand each directory level.

void VDMSConfig::expand_directory_layer(
    std::vector<std::vector<std::string> *> *p_directory_list,
    int current_layer) {
  std::vector<std::string> *tmp_directory_list = new std::vector<std::string>();
  if (current_layer > 1) {
    expand_directory_layer(p_directory_list, current_layer - 1);
  }
  if (p_directory_list->size() == 0) {
    for (int i = 0; i < DIRECTORIES_PER_LAYER; i++) {
      std::ostringstream tmp_stream;
      tmp_stream << std::internal << std::setfill('0')
                 << std::setw(CHARS_PER_LAYER_NAME) << i;
      tmp_directory_list->push_back(tmp_stream.str() + "/");
      // std::cout << (*tmp_directory_list)[i] << std::endl;
    }
    p_directory_list->push_back(tmp_directory_list);
  } else {
    for (int j = 0;
         j < (*p_directory_list)[p_directory_list->size() - 1]->size(); j++) {
      for (int i = 0; i < DIRECTORIES_PER_LAYER; i++) {
        std::ostringstream tmp_stream;
        tmp_stream << std::internal << std::setfill('0')
                   << std::setw(CHARS_PER_LAYER_NAME) << i;
        tmp_directory_list->push_back(
            (*(*p_directory_list)[p_directory_list->size() - 1])[j] +
            tmp_stream.str() + "/");
        // std::cout << (*tmp_directory_list)[tmp_directory_list->size() - 1] <<
        // std::endl;
      }
    }
    p_directory_list->push_back(tmp_directory_list);
  }
}

void VDMSConfig::create_directory_layer(
    std::vector<std::vector<std::string> *> *p_directory_list,
    std::string base_directory) {
  if (DIRECTORY_LAYERS > 0) {
    for (int i = 0; i < p_directory_list->size(); i++) {
      std::vector<std::string> *tmp_string_vector = (*p_directory_list)[i];
      for (int j = 0; j < tmp_string_vector->size(); j++) {
        check_or_create(base_directory + "/" + (*tmp_string_vector)[j]);
      }
    }
  }
}

// This method will check if the dir exists,
// and create the dir if it does not exist.
int VDMSConfig::create_dir(std::string path) {
  struct stat sb;
  while (1)
    if (stat(path.c_str(), &sb) == 0)
      if (sb.st_mode & S_IFDIR)
        return 0;
      else
        return EEXIST;
    else if (errno != ENOENT)
      return errno;
    else if (mkdir(path.c_str(), 0777) == 0)
      return 0;
    else if (errno != EEXIST)
      return errno;
}

void VDMSConfig::check_or_create(std::string path) {
  if (create_dir(path) == 0) {
    return;
  } else {
    std::cout << "Cannot open/create directories structure." << std::endl;
    std::cout << "Failed dir: " << path << std::endl;
    std::cout << "Check paths and permissions." << std::endl;
    std::cout << "Exiting..." << std::endl;
    exit(0);
  }
}

void VDMSConfig::build_dirs() {
  // Root
  path_root = get_string_value(PARAM_DB_ROOT, DEFAULT_PATH_ROOT);
  check_or_create(path_root);

  // PMGD
  path_pmgd = path_root + "/" + DEFAULT_PATH_PMGD;
  path_pmgd = get_string_value(PARAM_DB_PMGD, path_pmgd);
  check_or_create(path_pmgd);

  // IMAGES
  path_images = path_root + "/" + DEFAULT_PATH_IMAGES;
  path_images = get_string_value(PARAM_DB_IMAGES, path_images);
  check_or_create(path_images);

  std::vector<std::vector<std::string> *> directory_list;
  expand_directory_layer(&directory_list, DIRECTORY_LAYERS);

  // IMAGES - PNG
  path_png = path_images + "/" + DEFAULT_PATH_PNG;
  path_png = get_string_value(PARAM_DB_PNG, path_png);
  check_or_create(path_png);
  create_directory_layer(&directory_list, path_png);

  // IMAGES - JPG
  path_jpg = path_images + "/" + DEFAULT_PATH_JPG;
  path_jpg = get_string_value(PARAM_DB_JPG, path_jpg);
  check_or_create(path_jpg);
  create_directory_layer(&directory_list, path_jpg);

  // IMAGES - TDB
  path_tdb = path_images + "/" + DEFAULT_PATH_TDB;
  path_tdb = get_string_value(PARAM_DB_TDB, path_tdb);
  check_or_create(path_tdb);
  create_directory_layer(&directory_list, path_tdb);

  // IMAGES - BIN
  path_bin = path_images + "/" + DEFAULT_PATH_BIN;
  path_bin = get_string_value(PARAM_DB_BIN, path_bin);
  check_or_create(path_bin);
  create_directory_layer(&directory_list, path_bin);

  // BLOBS
  path_blobs = path_root + "/" + DEFAULT_PATH_BLOBS;
  path_blobs = get_string_value(PARAM_DB_BLOBS, path_blobs);
  check_or_create(path_blobs);
  create_directory_layer(&directory_list, path_blobs);

  // VIDEOS
  path_videos = path_root + "/" + DEFAULT_PATH_VIDEOS;
  path_videos = get_string_value(PARAM_DB_VIDEOS, path_videos);
  check_or_create(path_videos);
  create_directory_layer(&directory_list, path_videos);

  // DESCRIPTORS
  path_descriptors = path_root + "/" + DEFAULT_PATH_DESCRIPTORS;
  path_descriptors = get_string_value(PARAM_DB_DESCRIPTORS, path_descriptors);
  check_or_create(path_descriptors);

  // TMP
  path_tmp = "/tmp/" + std::string(DEFAULT_PATH_TMP);
  path_tmp = get_string_value(PARAM_DB_TMP, path_tmp);
  check_or_create(path_tmp);
  create_directory_layer(&directory_list, path_tmp);

  // get storage type, set use_aws flag
  storage_type = get_string_value(PARAM_STORAGE_TYPE, DEFAULT_STORAGE_TYPE);
  if (storage_type == DEFAULT_STORAGE_TYPE) {
    aws_flag = false;
  } else {
    aws_flag = true;
    aws_bucket_name = get_string_value(PARAM_BUCKET_NAME, DEFAULT_BUCKET_NAME);
  }
}
