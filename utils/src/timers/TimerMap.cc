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
#include "../../include/timers/TimerMap.h"
#include <iostream>

void TimerMap::add_timestamp(std::string timerid) {

  int element_count;

  maplock.lock();
  std::vector<std::chrono::steady_clock::time_point> *timestamps;
  // if the time ID already exists, extract the timestamp vector
  // and add a new timestamp
  element_count = timemap.count(timerid);
  if (element_count > 0) {
    timestamps = timemap.at(timerid);
  } else {
    // else create a new vector with chrono value
    timemap.insert(
        std::pair<std::string,
                  std::vector<std::chrono::steady_clock::time_point> *>(
            timerid, new std::vector<std::chrono::steady_clock::time_point>));
    timestamps = timemap.at(timerid);
  }
  timestamps->push_back(std::chrono::steady_clock::now());
  maplock.unlock();
}

std::vector<long long> TimerMap::return_all_timestamps(std::string timerid) {

  int element_count;
  std::chrono::steady_clock::time_point cur;
  std::vector<long long> timevals;
  std::vector<std::chrono::steady_clock::time_point> *timestamps;
  int i;
  long long timeval_us;

  element_count = timemap.count(timerid);
  if (element_count > 0) {
    timestamps = timemap.at(timerid);
    for (i = 0; i < timestamps->size(); i++) {
      cur = timestamps->at(i);
      timeval_us = std::chrono::time_point_cast<std::chrono::microseconds>(cur)
                       .time_since_epoch()
                       .count();
      timevals.push_back(timeval_us);
    }
  }

  return timevals;
}

std::vector<double> TimerMap::return_intervals(std::string timerid) {

  int element_count;
  double runtime;
  std::chrono::steady_clock::time_point cur, next;
  std::vector<double> intervals;

  element_count = timemap.count(timerid);
  if (element_count > 0) {
    std::vector<std::chrono::steady_clock::time_point> *timestamps =
        timemap.at(timerid);

    for (int i = 0; i < timestamps->size(); i++) {

      // If there's a timestampe "ahead", calculate the delta with that and
      // store it
      if (i < timestamps->size() - 1) {
        cur = timestamps->at(i);
        next = timestamps->at(i + 1);

        runtime =
            std::chrono::duration_cast<std::chrono::microseconds>(next - cur)
                .count();
        intervals.push_back(runtime);
      }
    }
  }
  return intervals;
}

double TimerMap::return_total_time(std::string timerid) {
  int element_count;
  std::chrono::steady_clock::time_point start, end;
  double runtime;

  element_count = timemap.count(timerid);
  if (element_count > 0) {
    std::vector<std::chrono::steady_clock::time_point> *timestamps =
        timemap.at(timerid);
    start = timestamps->front();
    end = timestamps->back();

    runtime = std::chrono::duration_cast<std::chrono::microseconds>(end - start)
                  .count();

    return runtime;
  } else {
    return 0.0;
  }
}

std::vector<std::string> TimerMap::return_ids() {

  std::vector<std::string> keylist;
  for (const auto &[key, value] : timemap) {
    keylist.push_back(key);
  }

  return keylist;
}

void TimerMap::clear_id(std::string timerid) {

  std::vector<std::chrono::steady_clock::time_point> *timestamps;
  auto it = timemap.find(timerid);
  if (it != timemap.end()) {
    timestamps = timemap.at(timerid);
    delete timestamps;
    timemap.erase(it);
  }
}

void TimerMap::print_map_runtimes() {

  std::vector<std::string> timer_id_list;
  // extract all runtimes, put into a JSON
  timer_id_list = return_ids();
  for (int i = 0; i < timer_id_list.size(); i++) {
    std::string cur_id = timer_id_list[i];
    std::cout << cur_id << " : " << return_total_time(cur_id) << std::endl;
  }
}

void TimerMap::clear_all_timers() { timemap.clear(); }

TimerMap::~TimerMap() {
  std::vector<std::string> timerids;
  timerids = this->return_ids();
  std::string cur_id;
  int i;

  for (i = 0; i < timerids.size(); i++) {
    cur_id = timerids[i];
    std::vector<std::chrono::steady_clock::time_point> *timestamps;
    timestamps = timemap.at(cur_id);
    delete timestamps;
    auto it = timemap.find(cur_id);
    if (it != timemap.end()) {
      timemap.erase(it);
    }
  }
}
