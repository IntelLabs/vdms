/**
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2017 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"),
 * to deal
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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * @section DESCRIPTION
 *
 */
using namespace VCL;

DescriptorParams::DescriptorParams(uint64_t numrows = 3,
                                   uint64_t cellsperrow = (1 << 12),
                                   uint64_t numhashtables = (1 << 9),
                                   uint64_t hashespertable = 14,
                                   uint64_t subhashbits = 2,
                                   uint64_t cutoff = 6) {
  this->num_rows = numrows;
  this->cells_per_row = cellsperrow;
  this->num_hash_tables = numhashtables;
  this->hashes_per_table = hashespertable;
  this->sub_hash_bits =
      subhashbits; // sub_hash_bits * hashes_per_table must be less than 32,
                   // otherwise segfault will happen
  this->cut_off = cutoff;
}
