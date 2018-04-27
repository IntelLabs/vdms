/**
 * @file   arch.h
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

#pragma once

#include <stdint.h>

template <typename T>
static inline void atomic_and(volatile T &m, T v)
  { asm volatile ("lock and%z0 %1, %0" : "+m"(m) : "ri"(v) : "memory"); }

template <typename T>
static inline bool bts(volatile T &m, int bit)
{
    bool result = 0;
    __asm__ volatile ("lock bts%z1 %2, %1\n; setc %0"
            : "=q"(result), "+m"(m) : "Ir"(T(bit)) : "memory", "cc");
    return result;
}

template <typename T>
static inline T xadd(volatile T &m, T v)
{
    T r = v;
    asm volatile ("lock xadd %1, %0" : "+m"(m), "+r"(r));
    return r;
}

static inline void pause()
{
    asm("pause");
}
