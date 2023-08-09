/**
 * @file   VDMSConfig.h
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

#pragma once

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <jsoncpp/json/value.h>

// Parameters in the JSON config file
#define PARAM_DB_ROOT "db_root_path"
#define PARAM_DB_PMGD "pmgd_path"
#define PARAM_DB_IMAGES "images_path"
#define PARAM_DB_PNG "png_path"
#define PARAM_DB_JPG "jpg_path"
#define PARAM_DB_TDB "tdb_path"
#define PARAM_DB_BIN "bin_path"
#define PARAM_DB_BLOBS "blobs_path"
#define PARAM_DB_VIDEOS "videos_path"
#define PARAM_DB_DESCRIPTORS "descriptors_path"
#define PARAM_DB_TMP "tmp_path"
#define PARAM_STORAGE_TYPE "storage_type"
#define PARAM_BUCKET_NAME "bucket_name"

#define PARAM_NODE_EXPIRATION "expiration_time"
#define DEFAULT_NODE_EXPIRATION 0

// Parameters used to determine depth and breadth of directory structure
// take parameters from command line if they are supplied
#ifndef DIRECTORIES_PER_LAYER
#define DIRECTORIES_PER_LAYER 5
#endif

#ifndef DIRECTORY_LAYERS
#define DIRECTORY_LAYERS 3
#endif

#ifndef CHARS_PER_LAYER_NAME
#define CHARS_PER_LAYER_NAME 3
#endif

#define PARAM_PMGD_NUM_ALLOCATORS "pmgd_num_allocators"
#define DEFAULT_PMGD_NUM_ALLOCATORS 1

namespace VDMS {

class VDMSConfig {

public:
  static bool init(std::string config_file);
  static void destroy();
  static VDMSConfig *instance();

private:
  static VDMSConfig *cfg;
  Json::Value json_config;

  // Dirs
  std::string path_root;
  std::string path_pmgd;
  std::string path_images;
  std::string path_png;
  std::string path_jpg;
  std::string path_bin;
  std::string path_tdb;
  std::string path_blobs;
  std::string path_videos;
  std::string path_descriptors;
  std::string path_tmp;
  std::string storage_type;

  bool aws_flag;               // use aws flag
  std::string aws_bucket_name; // aws bucket name

  VDMSConfig(std::string config_file);

  void expand_directory_layer(
      std::vector<std::vector<std::string> *> *p_directory_list,
      int current_layer);
  void create_directory_layer(
      std::vector<std::vector<std::string> *> *p_directory_list,
      std::string base_directory);
  void build_dirs();
  void check_or_create(std::string path);
  int create_dir(std::string path);

public:
  int get_int_value(std::string val, int def);
  std::string get_string_value(std::string val, std::string def);
  const std::string &get_path_root() { return path_root; }
  const std::string &get_path_pmgd() { return path_pmgd; }
  const std::string &get_path_jpg() { return path_jpg; }
  const std::string &get_path_png() { return path_png; }
  const std::string &get_path_bin() { return path_bin; }
  const std::string &get_path_tdb() { return path_tdb; }
  const std::string &get_path_blobs() { return path_blobs; }
  const std::string &get_path_videos() { return path_videos; }
  const std::string &get_path_descriptors() { return path_descriptors; }
  const std::string &get_path_tmp() { return path_tmp; }
  const std::string &get_storage_type() { return storage_type; }
  const std::string &get_bucket_name() { return aws_bucket_name; }
  const bool get_aws_flag() { return aws_flag; }
};

}; // namespace VDMS
