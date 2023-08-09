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

#include "sys/sysinfo.h"
#include "sys/times.h"
#include "sys/types.h"
#include "sys/vtimes.h"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include <chrono>
#include <ctime>
#include <fstream>

#include "SystemStats.h"

using namespace std;

static unsigned long long lastTotalUser, lastTotalUserLow, lastTotalSys,
    lastTotalIdle;
static clock_t lastCPU, lastSysCPU, lastUserCPU;
static int numProcessors;

// *****************************************************************************
// Public methods definitions
// *****************************************************************************

SystemStats::SystemStats() {
  memoryStats = {
      0, 0, 0, 0, 0, 0,
  };

  cpuStats = {
      0.0,
      0.0,
  };

  auto time_now = std::chrono::system_clock::now();
  std::chrono::duration<double> utc_time = time_now.time_since_epoch();

  logFileName = "/tmp/vdms_system_stats_" + std::to_string(utc_time.count());

  process_cpu_utilization_init();
  cpu_utilization_init();
}

SystemStats::~SystemStats(void) {}

// *****************************************************************************
// Memory Statistics
// *****************************************************************************

void SystemStats::get_system_virtual_memory() {
  struct sysinfo memoryInfo;
  sysinfo(&memoryInfo);

  long long totalVirtualMemory = memoryInfo.totalram;

  totalVirtualMemory += memoryInfo.totalswap;
  totalVirtualMemory *= memoryInfo.mem_unit;

  long long virtualMemoryUsed = memoryInfo.totalram - memoryInfo.freeram;

  virtualMemoryUsed += memoryInfo.totalswap - memoryInfo.freeswap;
  virtualMemoryUsed *= memoryInfo.mem_unit;

  memoryStats.total_virtual_memory = totalVirtualMemory;
  memoryStats.virtual_memory_used = virtualMemoryUsed;
}

int parseLine(char *line) {
  int i = strlen(line);
  const char *p = line;
  while (*p < '0' || *p > '9')
    p++;
  line[i - 3] = '\0';
  i = atoi(p);
  return i;
}

void SystemStats::get_process_virtual_memory() {
  FILE *file = fopen("/proc/self/status", "r");
  int virtualMemoryProcess = -1;
  char line[128];

  if (file != NULL) {
    while (fgets(line, 128, file) != NULL) {
      if (strncmp(line, "VmSize:", 7) == 0) {
        virtualMemoryProcess = parseLine(line);
        break;
      }
    }
    fclose(file);
  }

  memoryStats.virtual_memory_process = virtualMemoryProcess;
}

void SystemStats::get_system_physical_memory() {
  struct sysinfo memoryInfo;
  sysinfo(&memoryInfo);

  long long totalPhysicalMemory = memoryInfo.totalram;
  totalPhysicalMemory *= memoryInfo.mem_unit;

  long long physicalMemoryUsed = memoryInfo.totalram - memoryInfo.freeram;
  physicalMemoryUsed *= memoryInfo.mem_unit;

  memoryStats.total_physical_memory = totalPhysicalMemory;
  memoryStats.physical_memory_used = physicalMemoryUsed;
}

void SystemStats::get_process_physical_memory() {
  FILE *file = fopen("/proc/self/status", "r");
  int physicalMemoryProcess = -1;
  char line[128];

  if (file != NULL) {
    while (fgets(line, 128, file) != NULL) {
      if (strncmp(line, "VmRSS:", 6) == 0) {
        physicalMemoryProcess = parseLine(line);
        break;
      }
    }
    fclose(file);
  }

  memoryStats.physical_memory_process = physicalMemoryProcess;
}

// *****************************************************************************
// CPU Statistics
// *****************************************************************************

void SystemStats::cpu_utilization_init() {
  FILE *file = fopen("/proc/stat", "r");
  if (file != NULL) {
    if (fscanf(file, "cpu %llu %llu %llu %llu", &lastTotalUser,
               &lastTotalUserLow, &lastTotalSys, &lastTotalIdle) != 4) {
      printf("Error reading /proc/stats\n");
    }
    fclose(file);
  }
}

void SystemStats::process_cpu_utilization_init() {
  FILE *file;
  struct tms timeSample;
  char line[128];

  lastCPU = times(&timeSample);
  lastSysCPU = timeSample.tms_stime;
  lastUserCPU = timeSample.tms_utime;

  file = fopen("/proc/cpuinfo", "r");
  numProcessors = 0;
  if (file != NULL) {
    while (fgets(line, 128, file) != NULL) {
      if (strncmp(line, "processor", 9) == 0)
        numProcessors++;
    }
    fclose(file);
  }
}

void SystemStats::get_system_cpu_utilization() {
  double cpuUtilization;
  FILE *file;
  unsigned long long totalUser, totalUserLow, totalSys, totalIdle, total;

  file = fopen("/proc/stat", "r");

  if (file != NULL) {
    if (fscanf(file, "cpu %llu %llu %llu %llu", &totalUser, &totalUserLow,
               &totalSys, &totalIdle) != 4) {
      printf("Error reading /proc/stats\n");
    }
    fclose(file);

    if (totalUser < lastTotalUser || totalUserLow < lastTotalUserLow ||
        totalSys < lastTotalSys || totalIdle < lastTotalIdle) {
      // Overflow detection. Just skip this value.
      cpuUtilization = -1.0;
    } else {
      total = (totalUser - lastTotalUser) + (totalUserLow - lastTotalUserLow) +
              (totalSys - lastTotalSys);
      cpuUtilization = total;
      total += (totalIdle - lastTotalIdle);
      if (total != 0) {
        cpuUtilization /= total;
        cpuUtilization *= 100;
      } else {
        cpuUtilization = -1.0;
      }
    }

    lastTotalUser = totalUser;
    lastTotalUserLow = totalUserLow;
    lastTotalSys = totalSys;
    lastTotalIdle = totalIdle;
  } else {
    cpuUtilization = -1.0;
  }

  cpuStats.cpu_utilized = cpuUtilization;
}

void SystemStats::get_process_cpu_utilization() {
  struct tms timeSample;
  clock_t now;
  double cpuUtilization;

  now = times(&timeSample);
  if (now <= lastCPU || timeSample.tms_stime < lastSysCPU ||
      timeSample.tms_utime < lastUserCPU) {
    // Overflow detection. Just skip this value.
    cpuUtilization = -1.0;
  } else {
    cpuUtilization = (timeSample.tms_stime - lastSysCPU) +
                     (timeSample.tms_utime - lastUserCPU);
    // std::cout<< "Utilization Debug: " << cpuUtilization << " " <<
    // timeSample.tms_stime << " " << lastSysCPU << " " << timeSample.tms_utime
    // << " " << lastUserCPU << " " << now << " " << lastCPU << std::endl;
    cpuUtilization /= (now - lastCPU);
    cpuUtilization /= numProcessors;
    cpuUtilization *= 100;
  }
  lastCPU = now;
  lastSysCPU = timeSample.tms_stime;
  lastUserCPU = timeSample.tms_utime;

  cpuStats.cpu_utilized_process = cpuUtilization;
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

  statsFile.open(logFileName.data(), std::ios_base::app);

  auto time_now = std::chrono::system_clock::now();
  std::chrono::duration<double> utc_time = time_now.time_since_epoch();
  std::string currentTime = std::to_string(utc_time.count());

  statsFile << "Systems Statistics at " + currentTime + " for module " + pname +
                   "\n";
  statsFile << "Memory Statistics: \n";
  statsFile << "Total Virtual Memory: " +
                   std::to_string(memoryStats.total_virtual_memory) + "\n";
  statsFile << "Virtual Memory Used: " +
                   std::to_string(memoryStats.virtual_memory_used) + "\n";
  statsFile << "Virtual Memory Process: " +
                   std::to_string(memoryStats.virtual_memory_process) + "\n";
  statsFile << "Total Physical Memory: " +
                   std::to_string(memoryStats.total_physical_memory) + "\n";
  statsFile << "Physical Memory Used: " +
                   std::to_string(memoryStats.physical_memory_used) + "\n";
  statsFile << "Physical Memory Process: " +
                   std::to_string(memoryStats.physical_memory_process) + "\n";
  statsFile << "CPU Statistics: \n";
  statsFile << "Total CPU Utilization: " +
                   std::to_string(cpuStats.cpu_utilized) + "\n";
  statsFile << "Process CPU Utilization: " +
                   std::to_string(cpuStats.cpu_utilized_process) + "\n";
  statsFile << "\n";

  statsFile.close();
}