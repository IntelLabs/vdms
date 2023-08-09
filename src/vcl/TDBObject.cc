/**
 * @file   TDBObject.cc
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2017 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <cstring>
#include <errno.h>
#include <stddef.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "TDBObject.h"
#include "vcl/Exception.h"

using namespace VCL;

#define UCHAR_MAX 256

/*  *********************** */
/*        CONSTRUCTORS      */
/*  *********************** */

TDBObject::TDBObject() : _config(NULL) {
  _num_dimensions = 0;
  _tile_capacity = 0;
  _group = "";
  _name = "";

  // set default values
  _num_attributes = 1;
  const char *attr = "value";
  _attributes.push_back(attr);
  _compressed = CompressionType::LZ4;
  _min_tile_dimension = 4;
  _extent = -1;
}

TDBObject::TDBObject(const std::string &object_id) : _config(NULL) {
  _num_dimensions = 0;
  _tile_capacity = 0;

  size_t pos = get_path_delimiter(object_id);

  _group = get_group(object_id, pos);
  _name = get_name(object_id, pos);

  // set default values
  _num_attributes = 1;
  const char *attr = "value";
  _attributes.push_back(attr);
  _compressed = CompressionType::LZ4;
  _min_tile_dimension = 4;
  _extent = -1;
}

TDBObject::TDBObject(const std::string &object_id, RemoteConnection &connection)
    : _config(NULL) {
  set_config(&connection);

  _num_dimensions = 0;
  _tile_capacity = 0;

  size_t pos = get_path_delimiter(object_id);

  _group = get_group(object_id, pos);
  _name = get_name(object_id, pos);

  // set default values
  _num_attributes = 1;
  const char *attr = "value";
  _attributes.push_back(attr);
  _compressed = CompressionType::LZ4;
  _min_tile_dimension = 4;
  _extent = -1;
}

TDBObject::TDBObject(const TDBObject &tdb) : _config(NULL) {
  _num_dimensions = 0;
  _tile_capacity = 0;

  _config = tdb._config;
  _ctx = tdb._ctx;

  set_equal(tdb);
}

TDBObject &TDBObject::operator=(const TDBObject &tdb) {
  _config = tdb._config;
  _ctx = tdb._ctx;

  reset_arrays();

  set_equal(tdb);

  return *this;
}

void TDBObject::set_equal(const TDBObject &tdb) {
  _group = tdb._group;

  _num_attributes = tdb._num_attributes;

  _attributes.clear();
  _attributes = tdb._attributes;

  _num_dimensions = tdb._num_dimensions;
  _dimension_names.clear();
  _lower_dimensions.clear();
  _upper_dimensions.clear();

  _dimension_names = tdb._dimension_names;
  _upper_dimensions = tdb._upper_dimensions;
  _lower_dimensions = tdb._lower_dimensions;

  _compressed = tdb._compressed;
  _min_tile_dimension = tdb._min_tile_dimension;
  _array_dimension = tdb._array_dimension;
  _tile_dimension = tdb._tile_dimension;

  _config = tdb._config;
  _extent = tdb._extent;
}

TDBObject::~TDBObject() { reset_arrays(); }

void TDBObject::reset_arrays() {
  _attributes.clear();
  _attributes.shrink_to_fit();
  _dimension_names.clear();
  _dimension_names.shrink_to_fit();
  _lower_dimensions.clear();
  _upper_dimensions.clear();
  _lower_dimensions.shrink_to_fit();
  _upper_dimensions.shrink_to_fit();
}

void TDBObject::delete_object() {
  std::string object_id = _group + _name;
  tiledb::Object::remove(_ctx, object_id);
}

/*  *********************** */
/*        GET FUNCTIONS     */
/*  *********************** */

std::string TDBObject::get_object_id() const { return _group + _name; }

/*  *********************** */
/*        SET FUNCTIONS     */
/*  *********************** */

void TDBObject::set_num_dimensions(int num) { _num_dimensions = num; }

void TDBObject::set_dimension_names(
    const std::vector<std::string> &dimensions) {
  _num_dimensions = dimensions.size();
  _dimension_names = dimensions;
}

void TDBObject::set_dimension_lowerbounds(
    const std::vector<uint64_t> &dimensions) {
  _lower_dimensions = dimensions;
}

void TDBObject::set_dimension_upperbounds(
    const std::vector<uint64_t> &dimensions) {
  _upper_dimensions = dimensions;
}

template <class T>
void TDBObject::set_full_dimensions(const std::vector<std::string> &names,
                                    const std::vector<T> &upper_dims,
                                    const std::vector<T> &lower_dims,
                                    int extent) {
  _num_dimensions = names.size();
  for (int i = 0; i < names.size(); ++i) {
    auto dim = tiledb::Dimension::create<T>(
        _ctx, names[i], {{lower_dims[i], upper_dims[i]}}, extent);
    _full_dimensions.push_back(dim);
  }
}
template void
TDBObject::set_full_dimensions(const std::vector<std::string> &names,
                               const std::vector<uint64_t> &upper_dims,
                               const std::vector<uint64_t> &lower_dims,
                               int extent);
template void TDBObject::set_full_dimensions(
    const std::vector<std::string> &names, const std::vector<float> &upper_dims,
    const std::vector<float> &lower_dims, int extent);

void TDBObject::set_minimum(int dimension) { _min_tile_dimension = dimension; }

void TDBObject::set_num_attributes(int num) { _num_attributes = num; }

void TDBObject::set_attributes(const std::vector<std::string> &attributes) {
  _attributes.clear();
  std::vector<char *> charArrays;

  // Convert string values to C-style strings and store in charArrays
  for (auto x = 0; x < attributes.size(); ++x) {
    char *charArray =
        new char[attributes[x].length() + 1]; // +1 for null terminator
    std::strcpy(charArray, attributes[x].c_str());
    charArrays.push_back(charArray);
  }

  // Add the char arrays to _attributes vector
  for (auto x = 0; x < charArrays.size(); ++x) {
    _attributes.push_back(charArrays[x]);
  }
}

template <class T>
void TDBObject::set_single_attribute(std::string &attribute,
                                     CompressionType compressor,
                                     T cell_val_num) {

  tiledb::FilterList filter_list(_ctx);
  tiledb::Filter filter = convert_to_tiledb();
  filter_list.add_filter(filter);
  auto a = tiledb::Attribute::create<T>(_ctx, attribute, filter_list);
  a.set_cell_val_num((long)cell_val_num);
  _full_attributes.push_back(a);
}
template void TDBObject::set_single_attribute(std::string &attribute,
                                              CompressionType compressor,
                                              int cell_val_num);
template void TDBObject::set_single_attribute(std::string &attribute,
                                              CompressionType compressor,
                                              uint64_t cell_val_num);
template void TDBObject::set_single_attribute(std::string &attribute,
                                              CompressionType compressor,
                                              long cell_val_num);
template void TDBObject::set_single_attribute(std::string &attribute,
                                              CompressionType compressor,
                                              float cell_val_num);
template void TDBObject::set_single_attribute(std::string &attribute,
                                              CompressionType compressor,
                                              unsigned char cell_val_num);

void TDBObject::set_compression(CompressionType comp) { _compressed = comp; }

void TDBObject::set_config(RemoteConnection *remote) {
  // TODO: Implement this
}

/*  *********************** */
/*  PROTECTED GET FUNCTIONS */
/*  *********************** */

size_t TDBObject::get_path_delimiter(const std::string &filename) const {
  std::string delimiter = "/";

  size_t pos = filename.rfind(delimiter);
  if (pos == filename.length() - 1) {
    std::string file = filename.substr(0, pos);
    pos = file.rfind(delimiter);
  }

  return pos;
}

std::string TDBObject::get_group(const std::string &filename,
                                 size_t pos) const {
  std::string group = filename.substr(0, pos + 1);

  if (tiledb::Object::object(_ctx, group).type() !=
      tiledb::Object::Type::Group) {
    tiledb::create_group(_ctx, group);
  }

  return group;
}

std::string TDBObject::get_name(const std::string &filename, size_t pos) const {
  std::string id = filename.substr(pos + 1);
  return id;
}

/*  *********************** */
/*  PROTECTED SET FUNCTIONS */
/*  *********************** */
template <class T>
void TDBObject::set_schema_attributes(tiledb::ArraySchema &array_schema,
                                      std::vector<T> &cell_val_num) {
  if (_full_attributes.empty()) {
    for (int x = 0; x < _attributes.size(); ++x) {
      tiledb::FilterList filter_list(_ctx);
      filter_list.add_filter(convert_to_tiledb());

      auto attr =
          tiledb::Attribute::create<T>(_ctx, _attributes[x], filter_list);
      attr.set_cell_val_num(cell_val_num[x]);
      array_schema.add_attribute(attr);
    }
  } else {
    for (int x = 0; x < _full_attributes.size(); ++x) {
      array_schema.add_attribute(_full_attributes[x]);
    }
  }
}
template void
TDBObject::set_schema_attributes(tiledb::ArraySchema &array_schema,
                                 std::vector<unsigned char> &cell_val_num);
template void
TDBObject::set_schema_attributes(tiledb::ArraySchema &array_schema,
                                 std::vector<long> &cell_val_num);
template void
TDBObject::set_schema_attributes(tiledb::ArraySchema &array_schema,
                                 std::vector<uint64_t> &cell_val_num);
template void
TDBObject::set_schema_attributes(tiledb::ArraySchema &array_schema,
                                 std::vector<float> &cell_val_num);

void TDBObject::set_schema_dimensions(tiledb::ArraySchema &array_schema) {
  tiledb::Domain domain(_ctx);

  if (_extent == -1)
    find_tile_extents();
  else {
    _tile_dimension.clear();
    for (int x = 0; x < _num_dimensions; ++x) {
      _tile_dimension[x] = _extent;
    }
  }

  uint64_t domains[_num_dimensions][2];
  int y = 0;
  for (int x = 0; x < _num_dimensions; ++x) {
    if (!_lower_dimensions.empty())
      domains[x][0] = _lower_dimensions[y];
    else
      domains[x][0] = 0;
    domains[x][1] = _array_dimension[y];
    ++y;
  }

  for (int x = 0; x < _num_dimensions; ++x) {
    auto dim = tiledb::Dimension::create<uint64_t>(
        _ctx, _dimension_names[x].c_str(), {domains[x][0], domains[x][1]},
        _tile_dimension[x]);
    domain.add_dimension(dim);
  }

  array_schema.set_domain(domain);
}

void TDBObject::set_schema_domain(tiledb::ArraySchema &array_schema) {
  tiledb::Domain domain(_ctx);

  for (int x = 0; x < _full_dimensions.size(); ++x) {
    domain.add_dimension(_full_dimensions[x]);
  }

  array_schema.set_domain(domain);
}

template <class T>
void TDBObject::set_schema_dense(const std::string &object_id,
                                 std::vector<T> &cell_val_num, ORDER tile_order,
                                 ORDER data_order) {
  tiledb::ArraySchema array_schema(_ctx, TILEDB_DENSE);
  set_schema(cell_val_num, object_id, tile_order, data_order, array_schema);

  try {
    array_schema.check();
  } catch (tiledb::TileDBError &e) {
    throw VCLException(TileDBError, "Error creating TDB schema");
  }

  tiledb::Array::create(object_id, array_schema);
}
template void
TDBObject::set_schema_dense(const std::string &object_id,
                            std::vector<unsigned char> &cell_val_num,
                            ORDER tile_order, ORDER data_order);
template void TDBObject::set_schema_dense(const std::string &object_id,
                                          std::vector<long> &cell_val_num,
                                          ORDER tile_order, ORDER data_order);
template void TDBObject::set_schema_dense(const std::string &object_id,
                                          std::vector<uint64_t> &cell_val_num,
                                          ORDER tile_order, ORDER data_order);
template void TDBObject::set_schema_dense(const std::string &object_id,
                                          std::vector<float> &cell_val_num,
                                          ORDER tile_order, ORDER data_order);

template <class T>
void TDBObject::set_schema_sparse(const std::string &object_id,
                                  std::vector<T> &cell_val_num,
                                  ORDER tile_order, ORDER data_order) {
  tiledb::ArraySchema array_schema(_ctx, TILEDB_SPARSE);
  set_schema(cell_val_num, object_id, tile_order, data_order, array_schema);
  array_schema.set_capacity(_tile_capacity);

  try {
    array_schema.check();
  } catch (tiledb::TileDBError &e) {
    throw VCLException(TileDBError, "Error creating TDB schema");
  }

  tiledb::Array::create(object_id, array_schema);
}
template void
TDBObject::set_schema_sparse(const std::string &object_id,
                             std::vector<unsigned char> &cell_val_num,
                             ORDER tile_order, ORDER data_order);
template void TDBObject::set_schema_sparse(const std::string &object_id,
                                           std::vector<long> &cell_val_num,
                                           ORDER tile_order, ORDER data_order);
template void TDBObject::set_schema_sparse(const std::string &object_id,
                                           std::vector<uint64_t> &cell_val_num,
                                           ORDER tile_order, ORDER data_order);
template void TDBObject::set_schema_sparse(const std::string &object_id,
                                           std::vector<float> &cell_val_num,
                                           ORDER tile_order, ORDER data_order);

template <class T>
void TDBObject::set_schema(std::vector<T> &cell_val_num,
                           const std::string &object_id, ORDER tile_order,
                           ORDER data_order,
                           tiledb::ArraySchema &array_schema) {
  for (const T &value : cell_val_num) {
    // Access the value using the reference
  }

  if (tile_order == ORDER::ROW)
    array_schema.set_tile_order(TILEDB_ROW_MAJOR);
  else if (tile_order == ORDER::COLUMN)
    array_schema.set_tile_order(TILEDB_COL_MAJOR);
  else
    array_schema.set_tile_order(TILEDB_GLOBAL_ORDER);

  if (data_order == ORDER::ROW)
    array_schema.set_cell_order(TILEDB_ROW_MAJOR);
  else if (tile_order == ORDER::COLUMN)
    array_schema.set_cell_order(TILEDB_COL_MAJOR);
  else
    array_schema.set_tile_order(TILEDB_GLOBAL_ORDER);

  set_schema_attributes(array_schema, cell_val_num);

  if (_full_dimensions.empty())
    set_schema_dimensions(array_schema);
  else
    set_schema_domain(array_schema);
}
template void TDBObject::set_schema(std::vector<unsigned char> &cell_val_num,
                                    const std::string &object_id,
                                    ORDER tile_order, ORDER data_order,
                                    tiledb::ArraySchema &array_schema);
template void TDBObject::set_schema(std::vector<long> &cell_val_num,
                                    const std::string &object_id,
                                    ORDER tile_order, ORDER data_order,
                                    tiledb::ArraySchema &array_schema);
template void TDBObject::set_schema(std::vector<uint64_t> &cell_val_num,
                                    const std::string &object_id,
                                    ORDER tile_order, ORDER data_order,
                                    tiledb::ArraySchema &array_schema);
template void TDBObject::set_schema(std::vector<float> &cell_val_num,
                                    const std::string &object_id,
                                    ORDER tile_order, ORDER data_order,
                                    tiledb::ArraySchema &array_schema);

void TDBObject::set_from_schema(const std::string &object_id) {
  tiledb::ArraySchema array_schema(_ctx, object_id);

  _num_attributes = array_schema.attribute_num();

  tiledb::Domain domain = array_schema.domain();
  _num_dimensions = domain.ndim();

  std::vector<tiledb::Dimension> dimensions = domain.dimensions();
  for (int i = 0; i < dimensions.size(); ++i) {
    _tile_dimension.push_back(dimensions[i].tile_extent<uint64_t>());

    std::pair<uint64_t, uint64_t> domain = dimensions[i].domain<uint64_t>();
    _array_dimension.push_back(domain.second + 1);
    _upper_dimensions.push_back(domain.second + 1);
    _lower_dimensions.push_back(domain.first);
  }
}

/*  *********************** */
/*   METADATA INTERACTION   */
/*  *********************** */

template <class T>
void TDBObject::read_metadata(const std::string &array_name,
                              const std::vector<T> &subarray,
                              std::vector<uint64_t> &values,
                              std::string &attribute) {
  try {

    tiledb::Array array(_ctx, array_name, TILEDB_READ);
    tiledb::Query md_read(_ctx, array, TILEDB_READ);
    md_read.set_subarray(subarray);
    md_read.set_layout(TILEDB_ROW_MAJOR);

    std::vector<unsigned char> temp_values(values.size() * 2);
    md_read.set_data_buffer(attribute, temp_values);
    md_read.submit();
    array.close();

    int j = 0;
    for (int i = 0; i < temp_values.size(); ++i) {
      uint64_t val =
          (uint64_t)temp_values[i] * UCHAR_MAX + (uint64_t)temp_values[i + 1];
      values[j] = val;
      ++i;
      ++j;
    }
  } catch (tiledb::TileDBError &e) {
    throw VCLException(TileDBNotFound, "No data in TileDB object yet");
  }
}

template void TDBObject::read_metadata(const std::string &array_name,
                                       const std::vector<int> &subarray,
                                       std::vector<uint64_t> &values,
                                       std::string &attribute);
template void TDBObject::read_metadata(const std::string &array_name,
                                       const std::vector<uint64_t> &subarray,
                                       std::vector<uint64_t> &values,
                                       std::string &attribute);
template void TDBObject::read_metadata(const std::string &array_name,
                                       const std::vector<long> &subarray,
                                       std::vector<uint64_t> &values,
                                       std::string &attribute);

void TDBObject::find_tile_extents() {
  _array_dimension.clear();
  _tile_dimension.clear();
  for (int x = 0; x < _num_dimensions; ++x) {
    int dimension = _upper_dimensions[x] - _lower_dimensions[x];
    int num_tiles = 0;

    int gf_dimension = greatest_factor(dimension);

    while (gf_dimension == 1) {
      dimension = dimension + 1;
      gf_dimension = greatest_factor(dimension);
    }

    _array_dimension.push_back(dimension - _lower_dimensions[x]);
    _tile_dimension.push_back(gf_dimension);
  }
}

tiledb::Filter TDBObject::convert_to_tiledb() {
  tiledb::Filter f(_ctx, TILEDB_FILTER_ZSTD);

  int level = -1;

  switch (static_cast<int>(_compressed)) {
  case 0:
    f = tiledb::Filter(_ctx, TILEDB_FILTER_NONE);
    f.set_option(TILEDB_COMPRESSION_LEVEL, &level);
    break;
  case 1:
    f = tiledb::Filter(_ctx, TILEDB_FILTER_GZIP);
    f.set_option(TILEDB_COMPRESSION_LEVEL, &level);
    break;
  case 2:
    f = tiledb::Filter(_ctx, TILEDB_FILTER_ZSTD);
    f.set_option(TILEDB_COMPRESSION_LEVEL, &level);
    break;
  case 3:
    f = tiledb::Filter(_ctx, TILEDB_FILTER_LZ4);
    f.set_option(TILEDB_COMPRESSION_LEVEL, &level);
    break;

  case 4:
    f = tiledb::Filter(_ctx, TILEDB_FILTER_RLE);
    f.set_option(TILEDB_COMPRESSION_LEVEL, &level);
    break;
  case 5:
    f = tiledb::Filter(_ctx, TILEDB_FILTER_BZIP2);
    f.set_option(TILEDB_COMPRESSION_LEVEL, &level);
    break;
  case 6:
    f = tiledb::Filter(_ctx, TILEDB_FILTER_DOUBLE_DELTA);
    f.set_option(TILEDB_COMPRESSION_LEVEL, &level);
    break;
  default:
    throw VCLException(TileDBError, "Compression type not supported.\n");
  }

  return f;
}

/*  *********************** */
/*   PRIVATE FUNCTIONS      */
/*  *********************** */

void TDBObject::set_types(int *types) {
  for (int i = 0; i < _num_attributes; ++i) {
    types[i] = TILEDB_CHAR;
  }
}

int TDBObject::greatest_factor(int a) {
  int b = a;

  while (b > 1) {
    if (a % b == 0 && a / b >= _min_tile_dimension) {
      return b;
    }
    --b;
  }
  return b;
}
