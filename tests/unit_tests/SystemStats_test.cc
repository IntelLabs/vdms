/**
 * @file   SystemStats_test.cc
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2024 Intel Corporation
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
#include "sys/times.h"
#include <exception>
#include <stdio.h>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "VDMSConfig.h"
#include "stats/SystemStats.h"

using ::testing::_;
using ::testing::HasSubstr;
using ::testing::Return;
using ::testing::ThrowsMessage;

class SystemStatsTest : public ::testing::Test {

protected:
  virtual void SetUp() {}
  virtual void TearDown() {}
};

/* This class helps to mock some methods and also for accessing to the inherited
 * methods of the SystemStats class. Please notice that the mocked methods are
 * not wrapped into another method of this class
 */
class MockSystemStats : public SystemStats {
public:
  MockSystemStats() {}
  ~MockSystemStats() {}

  std::pair<TotalsInfo, bool> get_totals_info() {
    return SystemStats::get_totals_info();
  }

  int read_num_processors() { return SystemStats::read_num_processors(); }

  bool query_sufficient_memory(int size_requested) {
    return SystemStats::query_sufficient_memory(size_requested);
  }

  void set_memory_stats(const MemoryStats &memory_stats) {
    SystemStats::set_memory_stats(memory_stats);
  }

  void set_last_clocks_info(const ClocksInfo &last_clocks_info) {
    SystemStats::set_last_clocks_info(last_clocks_info);
  }

  void set_num_processors(int num) { SystemStats::set_num_processors(num); }

  CPUStats get_cpu_stats() { return SystemStats::get_cpu_stats(); }

  MemoryStats get_memory_stats() { return SystemStats::get_memory_stats(); }

  MOCK_METHOD(FILE *, get_stats_fd, (), (override));
  MOCK_METHOD(FILE *, get_proc_status, (), (override));
  MOCK_METHOD(FILE *, get_cpu_info_fd, (), (override));
  MOCK_METHOD(bool, is_overflow_in_time_detected,
              (clock_t now, struct tms time_sample), (override));
  MOCK_METHOD(void, get_system_virtual_memory, (), (override));
  MOCK_METHOD((std::pair<struct tms, clock_t>), get_time_info, (), (override));
};

/* This class helps to mock some methods and also for accessing to the inherited
 * methods of the SystemStats class. The difference with MockSystemStats class
 * is that there are some methods that could not be wrapped by MockSystemStats
 * class so they are included in this new AnotherMockSystemStats class
 */
class AnotherMockSystemStats : public SystemStats {
public:
  AnotherMockSystemStats() {}
  ~AnotherMockSystemStats() {}

  void set_last_totals_info(const TotalsInfo &last_totals_info) {
    SystemStats::set_last_totals_info(last_totals_info);
  }

  CPUStats get_cpu_stats() { return SystemStats::get_cpu_stats(); }

  ClocksInfo get_last_clocks_info() {
    return SystemStats::get_last_clocks_info();
  }

  MemoryStats get_memory_stats() { return SystemStats::get_memory_stats(); }

  MOCK_METHOD((std::pair<TotalsInfo, bool>), get_totals_info, (), (override));
  MOCK_METHOD(int, read_num_processors, (), (override));
  MOCK_METHOD((std::pair<struct tms, clock_t>), get_time_info, (), (override));
  MOCK_METHOD(struct sysinfo, get_sys_info, (), (override));
  MOCK_METHOD(bool, is_overflow_in_totals_detected,
              (const TotalsInfo &current_total_info,
               const TotalsInfo &last_totals_info),
              (override));
  MOCK_METHOD(FILE *, get_proc_status, (), (override));
  MOCK_METHOD(int, parse_line, (char *), (override));
};

/* This class helps as a wrapper of SystemStats class for calling to the
 * protected methods of the original class
 */
class SystemStatsModified : public SystemStats {
public:
  SystemStatsModified() {}
  ~SystemStatsModified() {}

  FILE *get_stats_fd() { return SystemStats::get_stats_fd(); }

  FILE *get_cpu_info_fd() { return SystemStats::get_cpu_info_fd(); }

  std::string get_filename_prefix() {
    return SystemStats::get_filename_prefix();
  }

  double get_epoch() { return SystemStats::get_epoch(); }

  struct sysinfo get_sys_info() {
    return SystemStats::get_sys_info();
  }

  FILE *get_proc_status() { return SystemStats::get_proc_status(); }

  std::pair<TotalsInfo, bool> get_totals_info() {
    return SystemStats::get_totals_info();
  }

  std::pair<struct tms, clock_t> get_time_info() {
    return SystemStats::get_time_info();
  }

  void set_num_processors(int num) { SystemStats::set_num_processors(num); }

  int get_num_processors() { return SystemStats::get_num_processors(); }

  int read_num_processors() { return SystemStats::read_num_processors(); }

  std::string get_log_filename() { return SystemStats::get_log_filename(); }

  void set_log_filename(const std::string &filename) {
    SystemStats::set_log_filename(filename);
  }

  bool is_overflow_in_time_detected(clock_t now, struct tms time_sample) {
    return SystemStats::is_overflow_in_time_detected(now, time_sample);
  }

  void set_last_clocks_info(const ClocksInfo &last_clocks_info) {
    SystemStats::set_last_clocks_info(last_clocks_info);
  }

  ClocksInfo get_last_clocks_info() {
    return SystemStats::get_last_clocks_info();
  }

  void set_memory_stats(const MemoryStats &memory_stats) {
    SystemStats::set_memory_stats(memory_stats);
  }

  TotalsInfo get_last_totals_info() {
    return SystemStats::get_last_totals_info();
  }

  void set_last_totals_info(const TotalsInfo &last_totals_info) {
    SystemStats::set_last_totals_info(last_totals_info);
  }

  MemoryStats get_memory_stats() { return SystemStats::get_memory_stats(); }

  void set_cpu_stats(const CPUStats &cpu_stats) {
    SystemStats::set_cpu_stats(cpu_stats);
  }

  CPUStats get_cpu_stats() { return SystemStats::get_cpu_stats(); }
};

TEST_F(SystemStatsTest, get_stats_fd_TEST) {
  try {
    // Prepare the test
    SystemStatsModified systemStats;

    // Execute the test
    FILE *file = systemStats.get_stats_fd();

    // Verify the results
    EXPECT_NE(file, nullptr);
    if (file) {
      fclose(file);
    }
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, get_cpu_info_fd_TEST) {
  try {
    // Prepare the test
    SystemStatsModified systemStats;

    // Execute the test
    FILE *file = systemStats.get_cpu_info_fd();

    // Verify the results
    EXPECT_NE(file, nullptr);
    if (file) {
      fclose(file);
    }
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, get_filename_prefix_TEST) {
  try {
    // Prepare the test
    SystemStatsModified systemStats;

    // Execute the test
    std::string filename_prefix = systemStats.get_filename_prefix();

    // Verify the results
    EXPECT_STREQ("vdms_system_stats_", filename_prefix.c_str());
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, get_epoch_TEST) {
  try {
    // Prepare the test
    SystemStatsModified systemStats;

    // Execute the test
    double epoch = systemStats.get_epoch();

    // Verify the results
    EXPECT_GT(epoch, 0);
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, get_sys_info_TEST) {
  try {
    // Prepare the test
    SystemStatsModified systemStats;

    // Execute the test
    struct sysinfo info = systemStats.get_sys_info();

    // Verify the results
    EXPECT_GT(info.uptime, 0);
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, get_proc_status_TEST) {
  try {
    // Prepare the test
    SystemStatsModified systemStats;

    // Execute the test
    FILE *file = systemStats.get_proc_status();

    // Verify the results
    EXPECT_NE(file, nullptr);
    if (file) {
      fclose(file);
    }
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, get_totals_info_TEST) {
  try {
    // Prepare the test
    SystemStatsModified systemStats;

    // Execute the test
    std::pair<TotalsInfo, bool> totals_info = systemStats.get_totals_info();

    // Verify the results
    EXPECT_TRUE(totals_info.second);
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, get_totals_info_WHEN_IS_NULL_TEST) {
  try {
    // Prepare the test
    MockSystemStats systemStats;
    EXPECT_CALL(systemStats, get_stats_fd()).WillOnce(Return(nullptr));

    // Execute the test
    std::pair<TotalsInfo, bool> totals_info = systemStats.get_totals_info();

    // Verify the results
    EXPECT_FALSE(totals_info.second);
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, get_totals_info_WHEN_IS_UNFORMATTED_TEST) {
  try {
    // Prepare the test
    MockSystemStats systemStats;
    std::string filename = "/tmp/get_totals_info_WHEN_IS_UNFORMATTED_TEST.txt";
    FILE *w_file = fopen(filename.c_str(), "w");
    if (w_file) {
      fwrite("test\n", sizeof(char), sizeof("test\n"), w_file);
      fclose(w_file);
    } else {
      FAIL() << "Error: file: " << filename.c_str() << " was not created";
    }

    FILE *r_file = fopen(filename.c_str(), "r");
    if (r_file) {
      EXPECT_CALL(systemStats, get_stats_fd()).WillOnce(Return(r_file));

      // Execute the test
      std::pair<TotalsInfo, bool> totals_info = systemStats.get_totals_info();
      int was_removed = std::remove(filename.c_str());
      EXPECT_TRUE(was_removed == 0);

      // Verify the results
      EXPECT_FALSE(totals_info.second);
    } else {
      FAIL() << "Error: file " << filename.c_str() << " was not read";
    }

  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, get_time_info_TEST) {
  try {
    // Prepare the test
    SystemStatsModified systemStats;

    // Execute the test
    std::pair<struct tms, clock_t> time_info = systemStats.get_time_info();

    // Verify the results
    EXPECT_GT(time_info.second, 0);
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, set_num_processors_TEST) {
  try {
    // Prepare the test
    SystemStatsModified systemStats;
    int num = 5;

    // Execute the test
    systemStats.set_num_processors(num);

    // Verify the results
    EXPECT_EQ(num, systemStats.get_num_processors());
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, read_num_processors_TEST) {
  try {
    // Prepare the test
    SystemStatsModified systemStats;

    // Execute the test
    int num = systemStats.read_num_processors();

    // Verify the results
    EXPECT_GT(num, 0);
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, read_num_processors_WITH_NULL_TEST) {
  try {
    // Prepare the test
    MockSystemStats systemStats;
    EXPECT_CALL(systemStats, get_cpu_info_fd()).WillOnce(Return(nullptr));

    // Execute the test and expect for an exception
    (void)systemStats.read_num_processors();
  } catch (std::exception &e) {
    // Verify the results
    EXPECT_STREQ(e.what(), "Error: get_cpu_info_fd() failed");
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, get_log_filename_TEST) {
  try {
    // Prepare the test
    SystemStatsModified systemStats;

    std::string expected_filename = "path/to/test/file";
    systemStats.set_log_filename(expected_filename);

    // Execute the test
    std::string filename = systemStats.get_log_filename();

    // Verify the results
    EXPECT_STREQ(filename.c_str(), expected_filename.c_str());
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, set_log_filename_TEST) {
  try {
    // Prepare the test
    SystemStatsModified systemStats;
    std::string expected_filename = "path/to/test/file";

    // Execute the test
    systemStats.set_log_filename(expected_filename);

    // Verify the results
    std::string filename = systemStats.get_log_filename();
    EXPECT_STREQ(filename.c_str(), expected_filename.c_str());
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, is_overflow_in_time_detected_TRUE_TEST) {
  try {
    // Prepare the test
    SystemStatsModified systemStats;
    clock_t now = 1;
    struct tms tmpTMS;
    tmpTMS.tms_utime = 0;
    tmpTMS.tms_stime = 0;
    tmpTMS.tms_cutime = 0;
    tmpTMS.tms_cstime = 0;

    // Prepare the test to make it return true
    ClocksInfo last_clocks_info;
    last_clocks_info.cpu = 2;
    systemStats.set_last_clocks_info(last_clocks_info);

    // Execute the test
    bool is_detected = systemStats.is_overflow_in_time_detected(now, tmpTMS);

    // Verify the results
    EXPECT_TRUE(is_detected);
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, is_overflow_in_time_detected_FALSE_TEST) {
  try {
    // Prepare the test
    SystemStatsModified systemStats;

    // Prepare the test to make it return false
    clock_t now = 3;

    struct tms tmpTMS;
    tmpTMS.tms_cutime = 0;
    tmpTMS.tms_cstime = 0;
    tmpTMS.tms_stime = 5;
    tmpTMS.tms_utime = 7;

    ClocksInfo last_clocks_info;
    last_clocks_info.userCPU = 6;
    last_clocks_info.sysCPU = 4;
    last_clocks_info.cpu = 2;
    systemStats.set_last_clocks_info(last_clocks_info);

    // Execute the test
    bool is_detected = systemStats.is_overflow_in_time_detected(now, tmpTMS);

    // Verify the results
    EXPECT_FALSE(is_detected);
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, set_memory_stats_TEST) {
  try {
    // Prepare the test
    SystemStatsModified systemStats;
    MemoryStats expected_memory_stats;
    expected_memory_stats.total_virtual_memory = 1;
    expected_memory_stats.virtual_memory_used = 2;
    expected_memory_stats.virtual_memory_process = 3;
    expected_memory_stats.total_physical_memory = 4;
    expected_memory_stats.physical_memory_used = 5;
    expected_memory_stats.physical_memory_process = 6;

    // Execute the test
    systemStats.set_memory_stats(expected_memory_stats);

    // Verify the results
    MemoryStats returned_memory_stats = systemStats.get_memory_stats();
    bool are_equal = expected_memory_stats == returned_memory_stats;
    EXPECT_TRUE(are_equal);
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, set_cpu_stats_TEST) {
  try {
    // Prepare the test
    SystemStatsModified systemStats;
    CPUStats expected_cpu_stats;
    expected_cpu_stats.cpu_utilized = 2;
    expected_cpu_stats.cpu_utilized_process = 3;

    // Execute the test
    systemStats.set_cpu_stats(expected_cpu_stats);

    // Verify the results
    CPUStats returned_cpu_stats = systemStats.get_cpu_stats();
    bool are_equal = expected_cpu_stats == returned_cpu_stats;
    EXPECT_TRUE(are_equal);
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, get_memory_stats_TEST) {
  try {
    // Prepare the test
    SystemStatsModified systemStats;
    MemoryStats expected_memory_stats;
    expected_memory_stats.total_virtual_memory = 2;
    expected_memory_stats.virtual_memory_used = 3;
    expected_memory_stats.virtual_memory_process = 4;
    expected_memory_stats.total_physical_memory = 5;
    expected_memory_stats.physical_memory_used = 6;
    expected_memory_stats.physical_memory_process = 7;

    systemStats.set_memory_stats(expected_memory_stats);

    // Execute the test
    MemoryStats returned_memory_stats = systemStats.get_memory_stats();

    // Verify the results
    bool are_equal = (expected_memory_stats == returned_memory_stats);
    EXPECT_TRUE(are_equal);
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, get_cpu_stats_TEST) {
  try {
    // Prepare the test
    SystemStatsModified systemStats;
    CPUStats expected_cpu_stats;
    expected_cpu_stats.cpu_utilized = 2;
    expected_cpu_stats.cpu_utilized_process = 3;

    systemStats.set_cpu_stats(expected_cpu_stats);

    // Execute the test
    CPUStats returned_cpu_stats = systemStats.get_cpu_stats();

    // Verify the results
    bool are_equal = (expected_cpu_stats == returned_cpu_stats);
    EXPECT_TRUE(are_equal);
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, set_last_totals_info_TEST) {
  try {
    // Prepare the test
    SystemStatsModified systemStats;
    TotalsInfo expected_info;
    expected_info.totalUser = 2;
    expected_info.totalUserLow = 3;
    expected_info.totalSys = 4;
    expected_info.totalIdle = 5;

    // Execute the test
    systemStats.set_last_totals_info(expected_info);

    // Verify the results
    TotalsInfo returned_info = systemStats.get_last_totals_info();
    bool are_equal = (expected_info == returned_info);
    EXPECT_TRUE(are_equal);
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, get_last_totals_info_TEST) {
  try {
    // Prepare the test
    SystemStatsModified systemStats;
    TotalsInfo expected_info;
    expected_info.totalUser = 2;
    expected_info.totalUserLow = 3;
    expected_info.totalSys = 4;
    expected_info.totalIdle = 5;

    systemStats.set_last_totals_info(expected_info);

    // Execute the test
    TotalsInfo returned_info = systemStats.get_last_totals_info();

    // Verify the results
    bool are_equal = (expected_info == returned_info);
    EXPECT_TRUE(are_equal);
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, set_last_clocks_info_TEST) {
  try {
    // Prepare the test
    SystemStatsModified systemStats;
    ClocksInfo expected_info;
    expected_info.cpu = 3;
    expected_info.sysCPU = 4;
    expected_info.userCPU = 5;

    // Execute the test
    systemStats.set_last_clocks_info(expected_info);

    // Verify the results
    ClocksInfo returned_info = systemStats.get_last_clocks_info();
    bool are_equal = (expected_info == returned_info);
    EXPECT_TRUE(are_equal);
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, get_last_clocks_info_TEST) {
  try {
    // Prepare the test
    SystemStatsModified systemStats;
    ClocksInfo expected_info;
    expected_info.cpu = 3;
    expected_info.sysCPU = 4;
    expected_info.userCPU = 5;

    systemStats.set_last_clocks_info(expected_info);

    // Execute the test
    ClocksInfo returned_info = systemStats.get_last_clocks_info();

    // Verify the results
    bool are_equal = (expected_info == returned_info);
    EXPECT_TRUE(are_equal);
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, cpu_utilization_init_TEST) {
  try {
    // Prepare the test
    AnotherMockSystemStats systemStats;
    TotalsInfo expected_info;
    expected_info.totalUser = 2;
    expected_info.totalUserLow = 3;
    expected_info.totalSys = 4;
    expected_info.totalIdle = 5;

    EXPECT_CALL(systemStats, get_totals_info())
        .WillOnce(Return((std::pair<TotalsInfo, bool>(expected_info, true))));

    // Execute the test
    TotalsInfo returned_info = systemStats.cpu_utilization_init();

    // Verify the results
    bool are_equal = (expected_info == returned_info);
    EXPECT_TRUE(are_equal);
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, cpu_utilization_init_WITH_FALSE_TEST) {
  try {
    // Prepare the test
    AnotherMockSystemStats systemStats;
    TotalsInfo expected_info;

    EXPECT_CALL(systemStats, get_totals_info())
        .WillOnce(Return((std::pair<TotalsInfo, bool>(expected_info, false))));

    // Execute the test and expect for an exception
    (void)systemStats.cpu_utilization_init();
  } catch (std::exception &e) {
    // Verify the results
    EXPECT_STREQ(e.what(), "Error calling to get_totals_info()");
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, process_cpu_utilization_init_TEST) {
  try {
    // Prepare the test
    AnotherMockSystemStats systemStats;

    struct tms timeSample;
    timeSample.tms_cutime = 0;
    timeSample.tms_cstime = 0;
    timeSample.tms_stime = 16;
    timeSample.tms_utime = 10;

    clock_t now = 20;

    int num_processors = 4;
    std::pair<struct tms, clock_t> time_info;
    time_info.first = timeSample;
    time_info.second = now;

    EXPECT_CALL(systemStats, get_time_info()).WillOnce(Return(time_info));
    EXPECT_CALL(systemStats, read_num_processors())
        .WillOnce(Return(num_processors));

    // Execute the test
    systemStats.process_cpu_utilization_init();

    // Verify the results
    ClocksInfo last_clocks_info = systemStats.get_last_clocks_info();
    EXPECT_TRUE(last_clocks_info.cpu == time_info.second);
    EXPECT_TRUE(last_clocks_info.sysCPU == time_info.first.tms_stime);
    EXPECT_TRUE(last_clocks_info.userCPU == time_info.first.tms_utime);
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, get_system_virtual_memory_TEST) {
  try {
    // Prepare the test
    AnotherMockSystemStats systemStats;

    struct sysinfo memoryInfo;
    memoryInfo.totalram = 1024;
    memoryInfo.totalswap = 512;
    memoryInfo.mem_unit = 2;

    memoryInfo.freeram = 256;
    memoryInfo.freeswap = 128;
    EXPECT_CALL(systemStats, get_sys_info()).WillOnce(Return(memoryInfo));

    long long expected_total_virtual_memory = 3072;
    long long expected_virtual_memory_used = 2304;

    // Execute the test
    systemStats.get_system_virtual_memory();

    // Verify the results
    MemoryStats memoryStats = systemStats.get_memory_stats();
    EXPECT_TRUE(memoryStats.total_virtual_memory ==
                expected_total_virtual_memory);

    EXPECT_TRUE(memoryStats.virtual_memory_used ==
                expected_virtual_memory_used);
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, get_process_virtual_memory_TEST) {
  try {
    // Prepare the test
    MockSystemStats systemStats;

    long long expected_virtual_memory_process = 8220;

    std::string filename = "/tmp/get_process_virtual_memory_TEST.txt";
    FILE *w_file = fopen(filename.c_str(), "w");
    if (w_file) {
      // Fills the file with the expected data
      fwrite("VmSize:     8220 kB\n", sizeof(char),
             sizeof("VmSize:     8220 kB\n"), w_file);
      if (w_file) {
        fclose(w_file);
      }
    } else {
      FAIL() << "Error: fopen(" << filename.c_str() << ", w) failed";
    }

    FILE *r_file = fopen(filename.c_str(), "r");
    if (r_file) {
      EXPECT_CALL(systemStats, get_proc_status()).WillOnce(Return(r_file));

      // Execute the test
      systemStats.get_process_virtual_memory();

      // Verify the results
      MemoryStats returned_memory_stats = systemStats.get_memory_stats();
      bool are_equal = returned_memory_stats.virtual_memory_process ==
                       expected_virtual_memory_process;

      int was_removed = std::remove(filename.c_str());
      EXPECT_TRUE(was_removed == 0);

      EXPECT_TRUE(are_equal);
    } else {
      FAIL() << "Error: fopen(" << filename.c_str() << ", r) failed";
    }
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, get_process_virtual_memory_WHEN_IS_NULL_TEST) {
  try {
    // Prepare the test
    MockSystemStats systemStats;
    EXPECT_CALL(systemStats, get_proc_status()).WillOnce(Return(nullptr));

    // Execute the test and expect an exception
    systemStats.get_process_virtual_memory();
  } catch (std::exception &e) {
    // Verify the results
    EXPECT_STREQ(e.what(), "Error: get_proc_status() failed");
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, get_process_virtual_memory_WHEN_LINE_IS_INVALID_TEST) {
  // Prepare the test
  std::string filename = "/tmp/get_process_virtual_memory_TEST.txt";
  try {
    // Prepare the test
    AnotherMockSystemStats systemStats;

    FILE *w_file = fopen(filename.c_str(), "w");
    if (w_file) {
      // Fills the file with the expected data
      fwrite("VmSize:     8220 kB\n", sizeof(char),
             sizeof("VmSize:     8220 kB\n"), w_file);
      if (w_file) {
        fclose(w_file);
      }
    } else {
      FAIL() << "Error: fopen(" << filename.c_str() << ", w) failed";
    }

    FILE *r_file = fopen(filename.c_str(), "r");
    if (r_file) {
      EXPECT_CALL(systemStats, get_proc_status()).WillOnce(Return(r_file));
      EXPECT_CALL(systemStats, parse_line(_)).WillOnce(Return(-1));

      // Execute the test and expect an exception
      systemStats.get_process_virtual_memory();

    } else {
      FAIL() << "Error: fopen(" << filename.c_str() << ", r) failed";
    }
  } catch (std::exception &e) {
    int was_removed = std::remove(filename.c_str());
    EXPECT_TRUE(was_removed == 0);

    // Verify the results
    EXPECT_TRUE(std::string(e.what()) == "Error: parse_line() failed");
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, get_system_physical_memory_TEST) {
  // Prepare the test
  try {
    AnotherMockSystemStats systemStats;

    long long expected_physical_memory_used = 1536;
    long long expected_total_physical_memory = 2048;

    struct sysinfo memoryInfo;
    memoryInfo.totalram = 1024;
    memoryInfo.mem_unit = 2;
    memoryInfo.freeram = 256;

    EXPECT_CALL(systemStats, get_sys_info()).WillOnce(Return(memoryInfo));

    // Execute the test
    systemStats.get_system_physical_memory();

    // Verify the results
    MemoryStats memoryStats = systemStats.get_memory_stats();
    EXPECT_TRUE(memoryStats.total_physical_memory ==
                expected_total_physical_memory);

    EXPECT_TRUE(memoryStats.physical_memory_used ==
                expected_physical_memory_used);
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, get_process_physical_memory_TEST) {
  try {
    // Prepare the test
    MockSystemStats systemStats;
    long long expected_physical_memory_process = 584;

    std::string filename = "/tmp/get_process_physical_memory_TEST.txt";
    FILE *w_file = fopen(filename.c_str(), "w");
    if (w_file) {
      // Fills the file with the expected data
      fwrite("VmRSS:       584 kB\n", sizeof(char),
             sizeof("VmRSS:       584 kB\n"), w_file);
      if (w_file) {
        fclose(w_file);
      }
    } else {
      FAIL() << "Error: fopen(" << filename.c_str() << ", w) failed";
    }

    FILE *r_file = fopen(filename.c_str(), "r");
    if (r_file) {
      EXPECT_CALL(systemStats, get_proc_status()).WillOnce(Return(r_file));

      // Execute the test
      systemStats.get_process_physical_memory();

      // Verify the results
      MemoryStats returned_memory_stats = systemStats.get_memory_stats();
      bool are_equal = returned_memory_stats.physical_memory_process ==
                       expected_physical_memory_process;

      int was_removed = std::remove(filename.c_str());
      EXPECT_TRUE(was_removed == 0);

      EXPECT_TRUE(are_equal);
    } else {
      FAIL() << "Error: fopen(" << filename.c_str() << ", r) failed";
    }
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, get_process_physical_memory_WHEN_IS_NULL_TEST) {
  try {
    // Prepare the test
    MockSystemStats systemStats;
    EXPECT_CALL(systemStats, get_proc_status()).WillOnce(Return(nullptr));

    // Execute the test and expect an exception
    systemStats.get_process_physical_memory();
  } catch (std::exception &e) {
    // Verify the results
    EXPECT_STREQ(
        e.what(),
        "Error: get_proc_status() failed in get_process_physical_memory()");
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, get_process_physical_memory_WHEN_LINE_IS_INVALID_TEST) {
  std::string filename = "/tmp/get_process_physical_memory_TEST.txt";

  try {
    // Prepare the test
    AnotherMockSystemStats systemStats;

    FILE *w_file = fopen(filename.c_str(), "w");
    if (w_file) {
      // Fills the file with the expected data
      fwrite("VmRSS:       584 kB\n", sizeof(char),
             sizeof("VmRSS:       584 kB\n"), w_file);
      if (w_file) {
        fclose(w_file);
      }
    } else {
      FAIL() << "Error: fopen(" << filename.c_str() << ", w) failed";
    }

    FILE *r_file = fopen(filename.c_str(), "r");
    if (r_file) {
      EXPECT_CALL(systemStats, get_proc_status()).WillOnce(Return(r_file));
      EXPECT_CALL(systemStats, parse_line(_)).WillOnce(Return(-1));

      // Execute the test
      systemStats.get_process_physical_memory();

    } else {
      FAIL() << "Error: fopen(" << filename.c_str() << ", r) failed";
    }
  } catch (std::exception &e) {
    int was_removed = std::remove(filename.c_str());
    EXPECT_TRUE(was_removed == 0);

    // Verify the results
    EXPECT_TRUE(std::string(e.what()) == "Error: parse_line() failed");
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, get_system_cpu_utilization_TEST) {
  try {
    // Prepare the test
    AnotherMockSystemStats systemStats;
    TotalsInfo current_info;
    current_info.totalUser = 4;
    current_info.totalUserLow = 5;
    current_info.totalSys = 6;
    current_info.totalIdle = 7;

    TotalsInfo last_info;
    last_info.totalUser = 3;
    last_info.totalUserLow = 4;
    last_info.totalSys = 5;
    last_info.totalIdle = 6;

    double expected_cpu_utilized = 75;

    systemStats.set_last_totals_info(last_info);

    EXPECT_CALL(systemStats, get_totals_info())
        .WillOnce(Return((std::pair<TotalsInfo, bool>(current_info, true))));

    EXPECT_CALL(systemStats, is_overflow_in_totals_detected(_, _))
        .WillOnce(Return(false));

    // Execute the test
    systemStats.get_system_cpu_utilization();

    // Verify the results
    CPUStats cpu_stats = systemStats.get_cpu_stats();
    EXPECT_DOUBLE_EQ(cpu_stats.cpu_utilized, expected_cpu_utilized);
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, get_system_cpu_utilization_WITH_TOTAL_ZERO_TEST) {
  try {
    // Prepare the test
    AnotherMockSystemStats systemStats;
    TotalsInfo current_info;
    current_info.totalUser = 3;
    current_info.totalUserLow = 4;
    current_info.totalSys = 5;
    current_info.totalIdle = 6;

    TotalsInfo last_info;
    last_info.totalUser = 3;
    last_info.totalUserLow = 4;
    last_info.totalSys = 5;
    last_info.totalIdle = 6;

    double expected_cpu_utilized = -1.0;

    systemStats.set_last_totals_info(last_info);

    EXPECT_CALL(systemStats, get_totals_info())
        .WillOnce(Return((std::pair<TotalsInfo, bool>(current_info, true))));

    EXPECT_CALL(systemStats, is_overflow_in_totals_detected(_, _))
        .WillOnce(Return(false));

    // Execute the test
    systemStats.get_system_cpu_utilization();

    // Verify the results
    CPUStats cpu_stats = systemStats.get_cpu_stats();
    EXPECT_DOUBLE_EQ(cpu_stats.cpu_utilized, expected_cpu_utilized);
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, get_system_cpu_utilization_WITH_OVERFLOW_TEST) {
  try {
    // Prepare the test
    AnotherMockSystemStats systemStats;
    TotalsInfo current_info;
    current_info.totalUser = 2;
    current_info.totalUserLow = 3;
    current_info.totalSys = 4;
    current_info.totalIdle = 5;

    double expected_cpu_utilized = -1.0;

    EXPECT_CALL(systemStats, get_totals_info())
        .WillOnce(Return((std::pair<TotalsInfo, bool>(current_info, true))));

    EXPECT_CALL(systemStats, is_overflow_in_totals_detected(_, _))
        .WillOnce(Return(true));

    // Execute the test
    systemStats.get_system_cpu_utilization();

    // Verify the results
    CPUStats cpu_stats = systemStats.get_cpu_stats();
    EXPECT_DOUBLE_EQ(cpu_stats.cpu_utilized, expected_cpu_utilized);
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, get_system_cpu_utilization_WITH_FALSE_TEST) {
  try {
    // Prepare the test
    AnotherMockSystemStats systemStats;
    TotalsInfo current_info;
    double expected_cpu_utilized = -1.0;

    EXPECT_CALL(systemStats, get_totals_info())
        .WillOnce(Return((std::pair<TotalsInfo, bool>(current_info, false))));

    // Execute the test
    systemStats.get_system_cpu_utilization();

    // Verify the results
    CPUStats cpu_stats = systemStats.get_cpu_stats();
    EXPECT_DOUBLE_EQ(cpu_stats.cpu_utilized, expected_cpu_utilized);
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, get_process_cpu_utilization_TEST) {
  try {
    // Prepare the test
    MockSystemStats systemStats;
    ClocksInfo clocks_info;
    clocks_info.cpu = 10;
    clocks_info.sysCPU = 3;
    clocks_info.userCPU = 3;

    struct tms timeSample;
    timeSample.tms_cutime = 0;
    timeSample.tms_cstime = 0;
    timeSample.tms_stime = 16;
    timeSample.tms_utime = 10;

    clock_t now = 20;

    int num_processors = 4;

    std::pair<struct tms, clock_t> time_info;
    time_info.first = timeSample;
    time_info.second = now;

    double expected_utilized = 50;

    systemStats.set_last_clocks_info(clocks_info);
    systemStats.set_num_processors(num_processors);
    EXPECT_CALL(systemStats, is_overflow_in_time_detected(_, _))
        .WillOnce(Return(false));
    EXPECT_CALL(systemStats, get_time_info()).WillOnce(Return(time_info));

    // Execute the test
    systemStats.get_process_cpu_utilization();

    // Verify the results
    CPUStats cpu_stats = systemStats.get_cpu_stats();
    EXPECT_DOUBLE_EQ(cpu_stats.cpu_utilized_process, expected_utilized);
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, log_stats_TEST) {
  // Prepare the test
  SystemStats systemStats;
  try {
    std::string pName = "log_stats_TEST";

    // Execute the test
    systemStats.log_stats(pName);

    // Verify the results
    FILE *file = fopen(systemStats.m_logFileName.data(), "r");
    ASSERT_TRUE(file != NULL);

    char anyTime[80];
    char expectedName[80];

    if (fscanf(file, "Systems Statistics at %s for module %s", anyTime,
               expectedName) != 2) {
      if (file) {
        fclose(file);
      }

      int was_removed = std::remove(systemStats.m_logFileName.c_str());
      EXPECT_TRUE(was_removed == 0);
      FAIL() << "Error: fscanf() was not able to get the values";
    }

    if (file) {
      fclose(file);
      int was_removed = std::remove(systemStats.m_logFileName.c_str());
      EXPECT_TRUE(was_removed == 0);
    }

    EXPECT_EQ(std::string(expectedName), pName);

  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, query_sufficient_memory_TEST) {
  try {
    // Prepare the test
    SystemStats systemStats;
    int size_requested = 0;

    // Execute the test
    bool result = systemStats.query_sufficient_memory(size_requested);

    // Verify the results
    EXPECT_TRUE(result);
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, query_sufficient_memory_WITH_ttlVirtMemMB_ZERO_TEST) {
  try {
    // Prepare the test
    MockSystemStats systemStats;
    int size_requested = 0;
    MemoryStats expected_memory_stats;
    expected_memory_stats.total_virtual_memory = 0;
    expected_memory_stats.virtual_memory_used = 0;

    systemStats.set_memory_stats(expected_memory_stats);

    EXPECT_CALL(systemStats, get_system_virtual_memory());

    // Execute the test an expect an exception
    (void)systemStats.query_sufficient_memory(size_requested);
  } catch (std::exception &e) {
    // Verify the results
    EXPECT_STREQ(e.what(), "Error: ttlVirtMemMB value is zero");
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}

TEST_F(SystemStatsTest, query_sufficient_memory_WITH_NOT_ENOUGH_MEMORY_TEST) {
  try {
    // Prepare the test
    MockSystemStats systemStats;
    int size_requested = 1;
    MemoryStats expected_memory_stats;
    expected_memory_stats.total_virtual_memory = 2 * 1024 * 1024;
    expected_memory_stats.virtual_memory_used = 2 * 1024 * 1024;

    systemStats.set_memory_stats(expected_memory_stats);

    EXPECT_CALL(systemStats, get_system_virtual_memory());

    // Execute the test
    bool is_available = systemStats.query_sufficient_memory(size_requested);

    // Verify the results
    EXPECT_FALSE(is_available);
  } catch (...) {
    FAIL() << "Error: Invalid exception";
  }
}