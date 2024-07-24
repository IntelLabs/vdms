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
#ifndef CLEAN_VDMS_TIMERMAP_H
#define CLEAN_VDMS_TIMERMAP_H

#include <chrono>
#include <map>
#include <mutex>
#include <string>
#include <vector>

class TimerMap {

private:
  std::map<std::string, std::vector<std::chrono::steady_clock::time_point> *>
      timemap;
  std::mutex maplock;

public:
  ~TimerMap();

  /**
   * Add a new timestamp at the current wall-clock time to a vector associated
   * with the specified timerID. If the timer id specified does not exist, it
   * will be created. It it already exists, a new timestamp will be added to the
   * associated ID
   * @param timerid c++ string identifying which time vector to append a
   * timestamp too.
   */
  void add_timestamp(std::string timerid);

  /**
   * Returns all timestamps associated with a particular timer ID, may
   * return empty vector if ID is not found
   * @param timerid string identifying a particular timestamp vector
   * @return vector of long longs representing wall-clock time in microseconds
   */
  std::vector<long long> return_all_timestamps(std::string timerid);

  /**
   * Return a vector of doubles for all time intervals in microseconds
   * associated with the specified time ID For example if there are 3 timestamps
   * affiliated with the specified timeid 2 times will be returned. If there are
   * 4, 3 will be returned. If there are zero or one times in the time vector,
   * an empty vector will be returned
   * @param timeid c++ string identifying which time vector to calculate
   * intervals over
   * @return possibly empty vector of return intervals in microseconds.
   */
  std::vector<double> return_intervals(std::string timerid);

  /**
   * Calculates the total time in microseconds between the first and
   * last captured timestamp for a particular time-id
   * @param timeid c++ string identifying which time vector to calculate total
   * time over
   * @return double in microseconds
   */
  double return_total_time(std::string timerid);

  /**
   * Used to get a list of all the current time-ids that are being tracked
   * @return vector of strings, each element being a timer ID with one of more
   * timestamps
   */
  std::vector<std::string> return_ids();

  /**
   * Deletes the specific timerID and its associated vector
   * @param timerid
   */
  void clear_id(std::string timerid);

  /**
   * Prints out the runtimes of the timer map to stdout
   */
  void print_map_runtimes();

  /**
   * Clears contents of timer map without destroying entrie structure
   */
  void clear_all_timers();
};

#endif // CLEAN_VDMS_TIMERMAP_H
