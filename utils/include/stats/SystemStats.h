/**
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

#include <iostream>
#include <stdint.h>
#include <string.h>
#include <string>

#include <time.h>

// ***************************************************************************
// SystemStats class
// ***************************************************************************

struct MemoryStats {
  long long total_virtual_memory;
  long long virtual_memory_used;
  long long virtual_memory_process;
  long long total_physical_memory;
  long long physical_memory_used;
  long long physical_memory_process;
};

struct CPUStats {
  double cpu_utilized;
  double cpu_utilized_process;
};

class SystemStats {
public:
  SystemStats();
  virtual ~SystemStats(void);

  MemoryStats memoryStats;
  CPUStats cpuStats;
  std::string logFileName;

  void cpu_utilization_init();
  void process_cpu_utilization_init();

  void get_system_virtual_memory();
  void get_process_virtual_memory();
  void get_system_physical_memory();
  void get_process_physical_memory();
  void get_system_cpu_utilization();
  void get_process_cpu_utilization();

  void log_stats(std::string pName);
};