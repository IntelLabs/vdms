/**
 * @file   TimerMapTest.cc
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
#include "timers/TimerMap.h"
#include "gtest/gtest.h"
#include <cstdlib>

class TimerMapTest : public ::testing::Test {
protected:
  // tests add and retrieval of timers based on ID
  void add_retrieve_test() {
    TimerMap timers;
    std::vector<std::string> id_list;
    std::vector<long long> timestamps;

    timers.add_timestamp("timer-1");
    sleep(1);
    timers.add_timestamp("timer-1");
    timers.add_timestamp("timer-2");
    timers.add_timestamp("timer-3");

    id_list = timers.return_ids();

    // Verify 3 timer IDs were returned
    ASSERT_EQ(id_list.size(), 3);

    // Now lets get time stamps out, and verify number of timestamps
    timestamps = timers.return_all_timestamps("timer-1");
    ASSERT_EQ(timestamps.size(), 2);

    timestamps = timers.return_all_timestamps("timer-2");
    ASSERT_EQ(timestamps.size(), 1);
    timestamps = timers.return_all_timestamps("timer-3");
    ASSERT_EQ(timestamps.size(), 1);
  }

  // Verifies we can add and remove a vector based on its ID, also explicitly
  // uses destructor
  // by making use of new/delete
  void clear_id_test() {
    TimerMap *timers = new TimerMap;
    std::vector<std::string> id_list;
    int rc_3;
    int rc_1;

    timers->add_timestamp("timer-1");
    timers->add_timestamp("timer-2");
    timers->add_timestamp("timer-3");
    rc_3 = timers->return_ids().size();

    // now lets delete two IDs
    timers->clear_id("timer-1");
    timers->clear_id("timer-3");
    rc_1 = timers->return_ids().size();

    // clear the timers
    delete timers;

    // Verify 3 timer IDs were returned
    ASSERT_EQ(rc_3, 3);
    ASSERT_EQ(rc_1, 1);
  }

  void runtime_test() {
    TimerMap timers;
    double total_runtime;

    timers.add_timestamp("timer-1");
    sleep(1);
    timers.add_timestamp("timer-1");

    total_runtime = timers.return_total_time("timer-1");

    ASSERT_TRUE((total_runtime < 1005000 && total_runtime > 950000));
  }

  void interval_test() {
    TimerMap timers;
    std::vector<double> intervals;

    timers.add_timestamp("timer-1");
    sleep(1);
    timers.add_timestamp("timer-1");
    sleep(1);
    timers.add_timestamp("timer-1");

    intervals = timers.return_intervals("timer-1");

    ASSERT_EQ(intervals.size(), 2);
  }

}; // end test class

TEST_F(TimerMapTest, AddRetrieveTest) { add_retrieve_test(); }
TEST_F(TimerMapTest, ClearIDTest) { clear_id_test(); }
TEST_F(TimerMapTest, RuntimeTest) { runtime_test(); }
TEST_F(TimerMapTest, IntervalTest) { interval_test(); }
