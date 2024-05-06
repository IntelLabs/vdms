/**
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2024 Intel Corporation
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
#include <stdio.h>
#include <string.h>
#include <string>
#include <time.h>

#include "sys/sysinfo.h"

/******************************* S T R U C T S ********************************/

struct MemoryStats {
  long long total_virtual_memory;
  long long virtual_memory_used;
  long long virtual_memory_process;
  long long total_physical_memory;
  long long physical_memory_used;
  long long physical_memory_process;

  MemoryStats() {
    total_virtual_memory = 0;
    virtual_memory_used = 0;
    virtual_memory_process = 0;
    total_physical_memory = 0;
    physical_memory_used = 0;
    physical_memory_process = 0;
  }

  bool operator==(MemoryStats &rhs) {
    bool result = true;
    result &= this->total_virtual_memory == rhs.total_virtual_memory;
    result &= this->virtual_memory_used == rhs.virtual_memory_used;
    result &= this->virtual_memory_process == rhs.virtual_memory_process;
    result &= this->total_physical_memory == rhs.total_physical_memory;
    result &= this->physical_memory_used == rhs.physical_memory_used;
    result &= this->physical_memory_process == rhs.physical_memory_process;

    return result;
  }

  bool operator!=(MemoryStats &rhs) { return *this != rhs; }
};

struct CPUStats {
  double cpu_utilized;
  double cpu_utilized_process;

  CPUStats() {
    cpu_utilized = 0.0;
    cpu_utilized_process = 0.0;
  }

  bool operator==(CPUStats &rhs) {
    bool result = true;
    result &= this->cpu_utilized == rhs.cpu_utilized;
    result &= this->cpu_utilized_process == rhs.cpu_utilized_process;
    return result;
  }

  bool operator!=(CPUStats &rhs) { return *this != rhs; }
};

struct TotalsInfo {
  unsigned long long totalUser;
  unsigned long long totalUserLow;
  unsigned long long totalSys;
  unsigned long long totalIdle;

  TotalsInfo() {
    totalUser = 0;
    totalUserLow = 0;
    totalSys = 0;
    totalIdle = 0;
  }

  bool operator==(TotalsInfo &rhs) {
    bool result = true;
    result &= this->totalUser == rhs.totalUser;
    result &= this->totalUserLow == rhs.totalUserLow;
    result &= this->totalSys == rhs.totalSys;
    result &= this->totalIdle == rhs.totalIdle;
    return result;
  }

  bool operator!=(TotalsInfo &rhs) { return *this != rhs; }
};

struct ClocksInfo {
  clock_t cpu;
  clock_t sysCPU;
  clock_t userCPU;

  ClocksInfo() {
    cpu = 0;
    sysCPU = 0;
    userCPU = 0;
  }

  bool operator==(ClocksInfo &rhs) {
    bool result = true;
    result &= this->cpu == rhs.cpu;
    result &= this->sysCPU == rhs.sysCPU;
    result &= this->userCPU == rhs.userCPU;
    return result;
  }

  bool operator!=(ClocksInfo &rhs) { return *this != rhs; }
};

// ***************************************************************************
// SystemStats class
// ***************************************************************************

class SystemStats {
public:
  SystemStats();
  virtual ~SystemStats();

  MemoryStats m_memoryStats;
  CPUStats m_cpuStats;
  std::string m_logFileName;

  virtual TotalsInfo cpu_utilization_init();
  virtual ClocksInfo process_cpu_utilization_init();

  virtual void get_system_virtual_memory();
  virtual void get_process_virtual_memory();
  virtual void get_system_physical_memory();
  virtual void get_process_physical_memory();
  virtual void get_system_cpu_utilization();
  virtual void get_process_cpu_utilization();

  virtual void log_stats(std::string pName);
  virtual bool query_sufficient_memory(int size_requested);

protected:
  virtual FILE *get_stats_fd();
  virtual FILE *get_cpu_info_fd();
  virtual std::string get_filename_prefix();
  virtual double get_epoch();
  virtual struct sysinfo get_sys_info();
  virtual FILE *get_proc_status();
  virtual std::pair<TotalsInfo, bool> get_totals_info();
  virtual std::pair<struct tms, clock_t> get_time_info();
  virtual void set_num_processors(int num);
  virtual int get_num_processors();
  virtual int read_num_processors();
  virtual std::string get_log_filename();
  virtual void set_log_filename(const std::string &filename);
  virtual bool is_overflow_in_time_detected(clock_t now,
                                            struct tms time_sample);

  virtual bool
  is_overflow_in_totals_detected(const TotalsInfo &current_total_info,
                                 const TotalsInfo &last_totals_info);

  virtual void set_memory_stats(const MemoryStats &memory_stats);
  virtual void set_cpu_stats(const CPUStats &cpu_stats);

  virtual MemoryStats get_memory_stats();
  virtual CPUStats get_cpu_stats();

  virtual ClocksInfo get_last_clocks_info();
  virtual void set_last_clocks_info(const ClocksInfo &last_clocks_info);

  virtual TotalsInfo get_last_totals_info();
  virtual void set_last_totals_info(const TotalsInfo &last_totals_info);

  TotalsInfo m_last_totals_info;
  ClocksInfo m_last_clocks_info;
  int m_numProcessors;

private:
  virtual int parse_line(char *line);
};
