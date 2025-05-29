/**
 * @file    CuckooCommon.h
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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * @section DESCRIPTION
 *
 * This file declares common data structures and constants for Cuckoo-based filters.
 *
 */

#ifndef CUCKOO_COMMON_H
#define CUCKOO_COMMON_H
#pragma once

#include "vcl/Filter.h" 

namespace VCL {

// The bucket struct for Hash Table filters common to CuckooHTFilter and CuckooCacheFilter.
// common header to centralize definition.
struct CACHE_ALIGNED filter_ht_bucket {
    filter_sig_t sigs[FILTER_BUCKET_ENTRIES];   /* 2-byte signature */
    filter_set_t sets[FILTER_BUCKET_ENTRIES];   /* 2-byte set */
};
} // namespace VCL

#endif // CUCKOO_COMMON_H