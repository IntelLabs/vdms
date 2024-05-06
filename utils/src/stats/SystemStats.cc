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

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <chrono>
#include <ctime>
#include <fstream>

#include "sys/times.h"
#include "sys/types.h"

#include "SystemStats.h"

using namespace std;

const std::string LOG_FILENAME_PREFIX = "vdms_system_stats_";

// *****************************************************************************
// Public methods definitions
// *****************************************************************************

SystemStats::SystemStats() {
  m_numProcessors = 0;
  std::string tmp_dir = "/tmp/"; // Could VDMS config file be called from utils?
  std::string filename =
      tmp_dir + get_filename_prefix() + std::to_string(get_epoch());
  set_log_filename(filename);
  process_cpu_utilization_init();
  cpu_utilization_init();
}

SystemStats::~SystemStats() {}

// *****************************************************************************
// Memory Statistics
// *****************************************************************************
void SystemStats::get_system_virtual_memory() {
  struct sysinfo memoryInfo = get_sys_info();

  long long totalVirtualMemory = memoryInfo.totalram;

  totalVirtualMemory += memoryInfo.totalswap;
  totalVirtualMemory *= memoryInfo.mem_unit;

  long long virtualMemoryUsed = memoryInfo.totalram - memoryInfo.freeram;

  virtualMemoryUsed += memoryInfo.totalswap - memoryInfo.freeswap;
  virtualMemoryUsed *= memoryInfo.mem_unit;

  m_memoryStats.total_virtual_memory = totalVirtualMemory;
  m_memoryStats.virtual_memory_used = virtualMemoryUsed;
}

void SystemStats::get_process_virtual_memory() {
  FILE *file = get_proc_status();
  int virtualMemoryProcess = -1;
  char line[128];

  if (file != NULL) {
    while (fgets(line, 128, file) != NULL) {
      if (strncmp(line, "VmSize:", 7) == 0) {
        virtualMemoryProcess = parse_line(line);
        if (-1 == virtualMemoryProcess) {
          if (file) {
            fclose(file);
          }
          throw std::runtime_error("Error: parse_line() failed");
        }
        break;
      }
    }
    if (file) {
      fclose(file);
    }
  } else {
    throw std::runtime_error("Error: get_proc_status() failed");
  }

  m_memoryStats.virtual_memory_process = virtualMemoryProcess;
}

void SystemStats::get_system_physical_memory() {
  struct sysinfo memoryInfo = get_sys_info();

  long long totalPhysicalMemory = memoryInfo.totalram;
  totalPhysicalMemory *= memoryInfo.mem_unit;

  long long physicalMemoryUsed = memoryInfo.totalram - memoryInfo.freeram;
  physicalMemoryUsed *= memoryInfo.mem_unit;

  m_memoryStats.total_physical_memory = totalPhysicalMemory;
  m_memoryStats.physical_memory_used = physicalMemoryUsed;
}

void SystemStats::get_process_physical_memory() {
  FILE *file = get_proc_status();

  int physicalMemoryProcess = -1;
  char line[128];

  if (file != NULL) {
    while (fgets(line, 128, file) != NULL) {
      if (strncmp(line, "VmRSS:", 6) == 0) {
        physicalMemoryProcess = parse_line(line);
        if (-1 == physicalMemoryProcess) {
          if (file) {
            fclose(file);
          }
          throw std::runtime_error("Error: parse_line() failed");
        }
        break;
      }
    }

    if (file) {
      fclose(file);
    }
  } else {
    throw std::runtime_error(
        "Error: get_proc_status() failed in get_process_physical_memory()");
  }

  m_memoryStats.physical_memory_process = physicalMemoryProcess;
}

// *****************************************************************************
// CPU Statistics
// *****************************************************************************
TotalsInfo SystemStats::cpu_utilization_init() {
  std::pair<TotalsInfo, bool> info = get_totals_info();

  // If there was an error when calling to get_totals_info()
  if (!info.second) {
    throw std::runtime_error("Error calling to get_totals_info()");
  }
  m_last_totals_info = info.first;
  return m_last_totals_info;
}

ClocksInfo SystemStats::process_cpu_utilization_init() {

  std::pair<struct tms, clock_t> time_info = get_time_info();
  m_last_clocks_info.cpu = time_info.second;
  m_last_clocks_info.sysCPU = time_info.first.tms_stime;
  m_last_clocks_info.userCPU = time_info.first.tms_utime;

  int num = read_num_processors();
  set_num_processors(num);

  return m_last_clocks_info;
}

void SystemStats::get_system_cpu_utilization() {
  double cpuUtilization = -1.0;

  std::pair<TotalsInfo, bool> info = get_totals_info();
  // If the TotalsInfo value returned by get_totals_info() is valid
  if (info.second) {
    TotalsInfo current_total_info = info.first;
    if (is_overflow_in_totals_detected(current_total_info,
                                       m_last_totals_info)) {
      // Overflow detection. Just skip this value.
      cpuUtilization = -1.0;
    } else {
      unsigned long long total =
          (current_total_info.totalUser - m_last_totals_info.totalUser) +
          (current_total_info.totalUserLow - m_last_totals_info.totalUserLow) +
          (current_total_info.totalSys - m_last_totals_info.totalSys);
      cpuUtilization = total;
      total += (current_total_info.totalIdle - m_last_totals_info.totalIdle);

      if (total != 0) {
        cpuUtilization /= total;
        cpuUtilization *= 100;
      } else {
        cpuUtilization = -1.0;
      }
    }

    m_last_totals_info.totalUser = current_total_info.totalUser;
    m_last_totals_info.totalUserLow = current_total_info.totalUserLow;
    m_last_totals_info.totalSys = current_total_info.totalSys;
    m_last_totals_info.totalIdle = current_total_info.totalIdle;
  } else {
    cpuUtilization = -1.0;
  }

  m_cpuStats.cpu_utilized = cpuUtilization;
}

void SystemStats::get_process_cpu_utilization() {

  std::pair<struct tms, clock_t> time_info = get_time_info();
  struct tms timeSample = time_info.first;
  clock_t now = time_info.second;

  double cpuUtilization = -1.0;

  if (is_overflow_in_time_detected(now, timeSample)) {
    // Overflow detection. Just skip this value.
    cpuUtilization = -1.0;
  } else {
    cpuUtilization = (timeSample.tms_stime - m_last_clocks_info.sysCPU) +
                     (timeSample.tms_utime - m_last_clocks_info.userCPU);
    // std::cout<< "Utilization Debug: " << cpuUtilization << " " <<
    // timeSample.tms_stime << " " << m_last_clocks_info.sysCPU << " " <<
    // timeSample.tms_utime
    // << " " << m_last_clocks_info.userCPU << " " << now << " " <<
    // m_last_clocks_info.cpu << std::endl;
    cpuUtilization /= (now - m_last_clocks_info.cpu);
    cpuUtilization /= m_numProcessors;
    cpuUtilization *= 100;
  }
  m_last_clocks_info.cpu = now;
  m_last_clocks_info.sysCPU = timeSample.tms_stime;
  m_last_clocks_info.userCPU = timeSample.tms_utime;

  m_cpuStats.cpu_utilized_process = cpuUtilization;
}

// *****************************************************************************
// Logging Functions
// *****************************************************************************
void SystemStats::log_stats(std::string pname) {
  get_system_virtual_memory();
  get_process_virtual_memory();
  get_system_physical_memory();
  get_process_physical_memory();
  get_system_cpu_utilization();
  get_process_cpu_utilization();

  std::ofstream statsFile;
  std::string filename = get_log_filename();
  statsFile.open(filename.data(), std::ios_base::app);

  std::string currentTime = std::to_string(get_epoch());

  statsFile << "Systems Statistics at " + currentTime + " for module " + pname +
                   "\n";
  statsFile << "Memory Statistics: \n";
  statsFile << "Total Virtual Memory: " +
                   std::to_string(m_memoryStats.total_virtual_memory) + "\n";
  statsFile << "Virtual Memory Used: " +
                   std::to_string(m_memoryStats.virtual_memory_used) + "\n";
  statsFile << "Virtual Memory Process: " +
                   std::to_string(m_memoryStats.virtual_memory_process) + "\n";
  statsFile << "Total Physical Memory: " +
                   std::to_string(m_memoryStats.total_physical_memory) + "\n";
  statsFile << "Physical Memory Used: " +
                   std::to_string(m_memoryStats.physical_memory_used) + "\n";
  statsFile << "Physical Memory Process: " +
                   std::to_string(m_memoryStats.physical_memory_process) + "\n";
  statsFile << "CPU Statistics: \n";
  statsFile << "Total CPU Utilization: " +
                   std::to_string(m_cpuStats.cpu_utilized) + "\n";
  statsFile << "Process CPU Utilization: " +
                   std::to_string(m_cpuStats.cpu_utilized_process) + "\n";
  statsFile << "\n";

  statsFile.close();
}

bool SystemStats::query_sufficient_memory(int size_requested) {
  get_system_virtual_memory();

  long conversion_B_MB = 1024 * 1024;
  long ttlVirtMemMB = m_memoryStats.total_virtual_memory / conversion_B_MB;
  long usedVirtMemMB = m_memoryStats.virtual_memory_used / conversion_B_MB;
  long availVirtMemMB = ttlVirtMemMB - usedVirtMemMB;

  if (0 == ttlVirtMemMB) {
    throw std::runtime_error("Error: ttlVirtMemMB value is zero");
  }

  float memPercent =
      (static_cast<float>(usedVirtMemMB) / static_cast<float>(ttlVirtMemMB)) *
      100;

  // cout << "TTL: " << ttlVirtMemMB << ", used: " << usedVirtMemMB << ", avail:
  // " << availVirtMemMB << ", requested: " << size_requested << endl; cout <<
  // "Used: " << memPercent << "%" << endl;

  printf("MEMORY: %0.1f%% used, %ldMB of %ldMB\n", memPercent, usedVirtMemMB,
         ttlVirtMemMB);

  if (size_requested < availVirtMemMB) {
    return true;
  }

  return false;
}

// *****************************************************************************
// Protected methods definitions
// *****************************************************************************
std::string SystemStats::get_filename_prefix() { return LOG_FILENAME_PREFIX; }

double SystemStats::get_epoch() {
  auto time_now = std::chrono::system_clock::now();
  std::chrono::duration<double> utc_time = time_now.time_since_epoch();
  return utc_time.count();
}

struct sysinfo SystemStats::get_sys_info() {
  struct sysinfo memoryInfo;
  sysinfo(&memoryInfo);
  return memoryInfo;
}

FILE *SystemStats::get_proc_status() {
  FILE *file = fopen("/proc/self/status", "r");
  return file;
}

FILE *SystemStats::get_stats_fd() { return fopen("/proc/stat", "r"); }

FILE *SystemStats::get_cpu_info_fd() { return fopen("/proc/cpuinfo", "r"); }

void SystemStats::set_cpu_stats(const CPUStats &cpuStats) {
  m_cpuStats = cpuStats;
}

CPUStats SystemStats::get_cpu_stats() { return m_cpuStats; }

MemoryStats SystemStats::get_memory_stats() { return m_memoryStats; }

void SystemStats::set_memory_stats(const MemoryStats &memoryStats) {
  m_memoryStats = memoryStats;
}

ClocksInfo SystemStats::get_last_clocks_info() { return m_last_clocks_info; }

void SystemStats::set_last_clocks_info(const ClocksInfo &last_clocks_info) {
  m_last_clocks_info = last_clocks_info;
}

TotalsInfo SystemStats::get_last_totals_info() { return m_last_totals_info; }

void SystemStats::set_last_totals_info(const TotalsInfo &last_totals_info) {
  m_last_totals_info = last_totals_info;
}

void SystemStats::set_log_filename(const std::string &filename) {
  m_logFileName = filename;
}

std::string SystemStats::get_log_filename() { return m_logFileName; }

// Return a tuple where the boolean is true if the TotalsInfo value is valid
std::pair<TotalsInfo, bool> SystemStats::get_totals_info() {
  TotalsInfo info;
  FILE *file = get_stats_fd();
  if (file != NULL) {
    if (fscanf(file, "cpu %llu %llu %llu %llu", &info.totalUser,
               &info.totalUserLow, &info.totalSys, &info.totalIdle) != 4) {
      printf("Error reading /proc/stats\n");
      if (file) {
        fclose(file);
      }
      return std::make_pair(info, false);
    }

    if (file) {
      fclose(file);
    }
  } else {
    return std::make_pair(info, false);
  }

  return std::make_pair(info, true);
}

std::pair<struct tms, clock_t> SystemStats::get_time_info() {
  struct tms time_sample;
  clock_t cpu_time = times(&time_sample);

  return std::make_pair(time_sample, cpu_time);
}

int SystemStats::read_num_processors() {
  int num = 0;
  FILE *file = get_cpu_info_fd();

  char line[128];
  if (file != NULL) {
    while (fgets(line, 128, file) != NULL) {
      if (strncmp(line, "processor", 9) == 0)
        num++;
    }

    if (file) {
      fclose(file);
    }
  } else {
    throw std::runtime_error("Error: get_cpu_info_fd() failed");
  }

  return num;
}

void SystemStats::set_num_processors(int num) { m_numProcessors = num; }

int SystemStats::get_num_processors() { return m_numProcessors; }

bool SystemStats::is_overflow_in_time_detected(clock_t now,
                                               struct tms time_sample) {
  return (now <= m_last_clocks_info.cpu ||
          time_sample.tms_stime < m_last_clocks_info.sysCPU ||
          time_sample.tms_utime < m_last_clocks_info.userCPU);
}

bool SystemStats::is_overflow_in_totals_detected(
    const TotalsInfo &current_total_info, const TotalsInfo &last_totals_info) {
  return (current_total_info.totalUser < last_totals_info.totalUser ||
          current_total_info.totalUserLow < last_totals_info.totalUserLow ||
          current_total_info.totalSys < last_totals_info.totalSys ||
          current_total_info.totalIdle < last_totals_info.totalIdle);
}

// *****************************************************************************
// Private methods definitions
// *****************************************************************************
int SystemStats::parse_line(char *line) {
  if (!line) {
    return -1;
  }

  int i = strlen(line);
  const char *p = line;
  while (*p < '0' || *p > '9')
    p++;
  line[i - 3] = '\0';
  i = atoi(p);
  return i;
}
