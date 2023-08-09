/**
 * @file   TDBObject.h
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
 * @section DESCRIPTION
 *
 * This file declares the C++ API for TDBObject
 */

#pragma once

#include <iostream>

#include <stdlib.h>
#include <string>
#include <vector>

#include "vcl/Exception.h"
#include "vcl/RemoteConnection.h"
#include "vcl/utils.h"
#include <tiledb/tiledb>

namespace VCL {

class TDBObject {

  /*  *********************** */
  /*        VARIABLES         */
  /*  *********************** */
protected:
  // Path variables
  std::string _group;
  std::string _name;

  // Dimensions (defines the type of TDBObject, should be set in inherited
  // class)
  int _num_dimensions;
  std::vector<std::string> _dimension_names;
  std::vector<uint64_t> _lower_dimensions;
  std::vector<uint64_t> _upper_dimensions;
  std::vector<tiledb::Dimension> _full_dimensions;
  std::vector<tiledb::Attribute> _full_attributes;

  // Attributes (number of values in a cell)
  int _num_attributes;
  std::vector<const char *> _attributes;

  // Compression type
  CompressionType _compressed;
  int _min_tile_dimension;

  int _extent;
  int _tile_capacity;

  // TileDB variables
  std::vector<uint64_t> _array_dimension;
  std::vector<uint64_t> _tile_dimension;
  tiledb::Context _ctx;

  tiledb::Config _config;

public:
  /*  *********************** */
  /*        ENUMS             */
  /*  *********************** */
  enum ORDER { ROW, COLUMN, GLOBAL };

  /*  *********************** */
  /*        CONSTRUCTORS      */
  /*  *********************** */
  /**
   *  Creates a empty TDBObject
   */
  TDBObject();

  /**
   *  Creates a TDBObject from an object id
   *
   *  @param object_id  The path of the TDBObject
   */
  TDBObject(const std::string &object_id);

  TDBObject(const std::string &object_id, RemoteConnection &connection);

  /**
   *  Creates a TDBObject from an existing TDBObject
   *
   *  @param tdb  A reference to an existing TDBObject
   */
  TDBObject(const TDBObject &tdb);

  /**
   *  Sets a TDBObject equal to another TDBObject
   *
   *  @param tdb  A reference to an existing TDBObject
   *  @return The current TDBObject
   */
  TDBObject &operator=(const TDBObject &tdb);

  /**
   *  TDBObject destructor
   */
  ~TDBObject();

  /*  *********************** */
  /*        GET FUNCTIONS     */
  /*  *********************** */
  /**
   *  Gets the path to the TDBObject
   *
   *  @return The string containing the full path to the TDBObject
   */
  std::string get_object_id() const;

  /*  *********************** */
  /*          SCHEMA          */
  /*  *********************** */
  /**
   *  Sets the number of dimensions in the TDBObject, specific
   *    to the type of TDBObject it will be (Vector objects have
   *    one dimension, Image objects have two dimensions,
   *    Volume objects have 3)
   *
   *  @param num_dimensions  The number of dimensions
   */
  void set_num_dimensions(int num_dimensions);

  /**
   *  Sets the names of the dimensions in the TDBObject
   *
   *  @param dimensions  A vector of strings that define the
   *    names of the dimensions
   */
  void set_dimension_names(const std::vector<std::string> &dimensions);

  /**
   *  Sets the values of the dimensions in the TDBObject
   *
   *  @param dimensions  A vector of integers that define the
   *    largest value of each dimension
   */
  void set_dimension_upperbounds(const std::vector<uint64_t> &dimensions);

  /**
   *  Sets the values of the dimensions in the TDBObject
   *
   *  @param dimensions  A vector of integers that define the
   *    smallest value of each dimension
   */
  void set_dimension_lowerbounds(const std::vector<uint64_t> &dimensions);

  /**
   *  Sets dimensions for the TDBObject using TileDB Dimensions
   *
   *  @param names  A vector of names for each dimension
   *  @param upper_dims A vector of the upper value for each dimension
   *  @param lower_dims A vector of the lower value for each dimension
   *  @param extent  The tile extent to use
   *  @note Use this when your domains are not integer values
   */
  template <class T>
  void set_full_dimensions(const std::vector<std::string> &names,
                           const std::vector<T> &upper_dims,
                           const std::vector<T> &lower_dims, int extent);

  /**
   *  Sets an attribute for the TDBObject using TileDB Attributes
   *
   *  @param attribute  The name of the attribute
   *  @param compressor The type of compression to use
   *  @param cell_val_num The number of values per cell, in the data type the
   * attribute should be
   *  @note Use this when you want to have different data types for each
   * attribute
   */
  template <class T>
  void set_single_attribute(std::string &attribute, CompressionType compressor,
                            T cell_val_num);

  /**
   *  Sets the minimum tile dimension
   *
   *  @param min  The minimum number of tiles per dimension
   */
  void set_minimum(int dimension);

  /**
   *  Sets the number of attributes in the TDBObject, which defines
   *    how the array is stored. Default is usually one
   *
   *  @param num_attributes  The number of attributes
   */
  void set_num_attributes(int num_attributes);

  /**
   *  Sets the names of attributes in the TDBObject
   *
   *  @param attributes  A vector of strings that define the
   *    names of the attributes
   */
  void set_attributes(const std::vector<std::string> &attributes);

  /**
   *  Sets the type of compression to be used when compressing
   *    the TDBObject
   *
   *  @param comp  The compression type
   *  @see Image.h for details on CompressionType
   */
  void set_compression(CompressionType comp);

  void set_config(RemoteConnection *remote);

  /**
   *  Sets the tile extents in the TDBObject
   *
   *  @param extent  The tile extent
   */
  void set_extent(int extent) { _extent = extent; };

  /**
   *  Sets the tile capacity in a sparse TDBObject
   *
   *  @param capacity  The tile capacity
   */
  void set_capacity(int capacity) { _tile_capacity = capacity; };

  /**
   *  Determines the TileDB schema variables and sets the
   *    schema for writing a dense TileDB array
   *
   *  @param  object_id  The full path to the TileDB array
   *  @param  cell_val_num  The number of values per cell in the array
   *  @param  tile_order  The order in which to store tiles (row, column)
   *  @param  data_order  The order in which to store data within a tile (row,
   * column)
   */
  template <class T>
  void set_schema_dense(const std::string &object_id,
                        std::vector<T> &cell_val_num,
                        ORDER tile_order = ORDER::ROW,
                        ORDER data_order = ORDER::ROW);

  /**
   *  Determines the TileDB schema variables and sets the
   *    schema for writing a sparse TileDB array
   *
   *  @param  object_id  The full path to the TileDB array
   *  @param  cell_val_num  The number of values per cell in the array
   *  @param  tile_order  The order in which to store tiles (row, column)
   *  @param  data_order  The order in which to store data within a tile (row,
   * column)
   */
  template <class T>
  void set_schema_sparse(const std::string &object_id,
                         std::vector<T> &cell_val_num,
                         ORDER tile_order = ORDER::ROW,
                         ORDER data_order = ORDER::ROW);

  /*  *********************** */
  /*  TDBOBJECT INTERACTION   */
  /*  *********************** */

  /**
   *  Deletes the object from TileDB
   */
  void delete_object();

  /*  *********************** */
  /*   METADATA INTERACTION   */
  /*  *********************** */

  /**
   *  Reads the TDBObject metadata
   *
   *  @param  array_name  The full path to the TileDB array
   *  @param  subarray  A vector indicating where in the array
   *               the metadata is stored
   *  @param  values  A vector in which to store the metadata values
   */
  template <class T>
  void read_metadata(const std::string &metadata,
                     const std::vector<T> &subarray,
                     std::vector<uint64_t> &values, std::string &attribute);

protected:
  /*  *********************** */
  /*        GET FUNCTIONS     */
  /*  *********************** */
  /**
   *  Gets the location of the last / in an object id
   *
   *  @param  object_id  A string
   *  @return The location of the last / in the given string
   */
  size_t get_path_delimiter(const std::string &object_id) const;

  /**
   *  Gets the parent directory of a file (the TileDB group)
   *    and tries to create the directory if it does not exist
   *
   *  @param  filename  The full path of the file
   *  @param  pos  The location of the last / in the filename
   *  @return The name of the TileDB group
   */
  std::string get_group(const std::string &filename, size_t pos) const;

  /**
   *  Gets the name of a file (the TileDB array)
   *
   *  @param  filename  The full path of the file
   *  @param  pos  The location of the last / in the filename
   *  @return The name of the TileDB array
   */
  std::string get_name(const std::string &filename, size_t pos) const;

  /*  *********************** */
  /*        SET FUNCTIONS     */
  /*  *********************** */
  /**
   *  Sets the member variables of one TDBObject equal to another
   *
   *  @param  tdb  The TDBOjbect to set the current TDBObject's
   *    variables equal to
   */
  void set_equal(const TDBObject &tdb);

  /**
   *  Sets the TDBObject values from an array schema
   *
   *  @param  object_id  The full path to the TileDB array
   */
  void set_from_schema(const std::string &object_id);

private:
  /**
   *  Sets the TileDB type of the attribute values, currently
   *    all are unsigned characters
   *
   *  @param  types  An array to be filled with the attribute
   *    value types
   */
  void set_types(int *types);

  /**
   *  Finds the greatest factor of a number
   *
   *  @param a  The number to factor
   *  @return  The greatest factor of a
   */
  int greatest_factor(int a);

  /**
   *  Resets the arrays that are members of this class
   */
  void reset_arrays();

  /**
   *  Sets the TileDB schema dimensions to the appropriate values
   *
   *  @param array_schema  The TileDB array schema
   */
  void set_schema_dimensions(tiledb::ArraySchema &array_schema);

  /**
   *  Sets the TileDB schema domain
   *
   *  @param array_schema  The TileDB array schema
   */
  void set_schema_domain(tiledb::ArraySchema &array_schema);

  /**
   *  Sets the TileDB schema attributes to the appropriate values
   *
   *  @param array_schema  The TileDB array schema
   *  @param cell_val_num  The number of values per cell
   */
  template <class T>
  void set_schema_attributes(tiledb::ArraySchema &array_schema,
                             std::vector<T> &cell_val_num);

  /**
   *  Sets the TileDB schema
   *
   *  @param cell_val_num  The number of values per cell
   *  @param object_id  The full path to the TileDB array
   *  @param array_schema  The TileDB array schema
   */
  template <class T>
  void set_schema(std::vector<T> &cell_val_num, const std::string &object_id,
                  ORDER tile_order, ORDER data_order,
                  tiledb::ArraySchema &array_schema);

  /**
   *  Converts the VCL CompressionType to TileDB compression
   */
  tiledb::Filter convert_to_tiledb();

  /**
   *  Determines the size of the TDBObject array as well as
   *    the size of the tiles. Currently tiles have the same
   *    length in all dimensions, and the minimum number of
   *    tiles is 100
   */
  void find_tile_extents();
};
}; // namespace VCL
