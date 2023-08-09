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

#include <cmath>
#include <iostream>
#include <sstream>

#include <sys/time.h>

#include "Chrono.h"

using namespace std;

// *****************************************************************************
// Public methods definitions
// *****************************************************************************
Chrono::Chrono(const string &name, const bool asyncEnabled)
    : name(name), enabled(true) {
  elapsedStats.name = "elapsedStats";
  periodStats.name = "periodStats";
  reset();
}

Chrono::Chrono() : Chrono("no_name") {}

Chrono::~Chrono(void) {}

void Chrono::tic(void) {
  if (!enabled) {
    return;
  }

  if (ticIdle) {
    ticIdle = false;
    doTic();
  } else {
    ++errors;
    cerr << "Chrono::tic - " << name
         << ": Calling Chrono::tic with no matching Chrono::tag!" << endl;
  }
}

void Chrono::tac(void) {
  if (!enabled) {
    return;
  }

  if (!ticIdle) {
    ticIdle = true;
    doTac();
  } else {
    ++errors;
    cerr << "Chrono::tac - " << name
         << ": Calling Chrono::tac with no matching Chrono::tic!" << endl;
  }
}

void Chrono::reset(void) {
  ticIdle = true;
  errors = 0;
  resetStats(elapsedStats);
  resetStats(periodStats);
}

void Chrono::setEnabled(const bool val) { enabled = val; }

std::ostream &Chrono::printStats(const Chrono::ChronoStats &stats,
                                 std::ostream &os) const {
  os.precision(2);
  os << fixed;
  os << name << ": " << stats.name << endl;
  os << "\terrors: " << errors << endl;
  os << "\ttotalTime: " << stats.totalTime_ms << " [ms]" << endl;
  os << "\taverageTime: " << stats.averageTime_ms << " [ms]" << endl;
  os << "\tstdDevTime: " << stats.stdDevTime_ms << " [ms]" << endl;
  os << "\tlastTime: " << stats.lastTime_ms << " [ms]" << endl;
  os << "\tminTime: " << stats.minTime_ms << " [ms]" << endl;
  os << "\tmaxTime: " << stats.maxTime_ms << " [ms]" << endl;

  return os;
}

std::ostream &Chrono::printAvgTime(const Chrono::ChronoStats &stats,
                                   std::ostream &os) const {
  os.precision(2);
  os << fixed;
  os << name << ": " << stats.name << " -> "
     << "averageTime: " << stats.averageTime_ms << " [ms]" << endl;

  return os;
}

std::ostream &Chrono::printAvgTime(const Chrono::ChronoStats &stats,
                                   std::ostream &os, const float ref) const {
  os.precision(2);
  os << fixed;
  os << name << ": " << stats.name << " -> "
     << "averageTime: " << stats.averageTime_ms << " [ms] (";
  os << (stats.averageTime_ms / ref * 100.0f) << "%)" << endl;

  return os;
}

// *****************************************************************************
// Private/Protected methods definitions
// *****************************************************************************
void Chrono::resetStats(ChronoStats &stats) {
  stats.counter = 0;
  stats.totalTime_ms = 0.0f;
  stats.totalSquaredTime_ms2 = 0.0f;
  stats.averageTime_ms = 0.0f;
  stats.stdDevTime_ms = 0.0f;
  stats.lastTime_ms = 0.0f;
  stats.minTime_ms = 0.0f;
  stats.maxTime_ms = 0.0f;
}

void Chrono::updateStats(ChronoStats &stats) {
  ++stats.counter;
  stats.totalTime_ms += stats.lastTime_ms;
  stats.totalSquaredTime_ms2 += stats.lastTime_ms * stats.lastTime_ms;
  stats.averageTime_ms = stats.totalTime_ms / (float)stats.counter;
  stats.stdDevTime_ms =
      sqrtf(stats.totalSquaredTime_ms2 / (float)stats.counter -
            stats.averageTime_ms * stats.averageTime_ms);
  if (stats.counter > 1) {
    stats.maxTime_ms = max(stats.lastTime_ms, stats.maxTime_ms);
    stats.minTime_ms = min(stats.lastTime_ms, stats.minTime_ms);
  } else {
    stats.maxTime_ms = stats.lastTime_ms;
    stats.minTime_ms = stats.lastTime_ms;
  }
}

// *****************************************************************************
// ChronoCpu Implementation
// *****************************************************************************

ChronoCpu::ChronoCpu(const string &name) : Chrono(name), ticCounter(0) {
  memset((void *)&lastTicTime, 0, sizeof(lastTicTime));
  memset((void *)&ticTime, 0, sizeof(ticTime));
  memset((void *)&tacTime, 0, sizeof(tacTime));

#ifdef __MACH__ // OS X does not have clock_gettime, use clock_get_time
  host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
#endif
}

ChronoCpu::~ChronoCpu(void) {
#ifdef __MACH__ // OS X does not have clock_gettime, use clock_get_time
  mach_port_deallocate(mach_task_self(), cclock);
#endif
}

ChronoCpu::ChronoCpu() : ChronoCpu("no_name") {}

// *****************************************************************************
// Private/Protected methods definitions
// *****************************************************************************
void ChronoCpu::doTic(void) {
  lastTicTime = ticTime;

#ifdef __MACH__ // OS X does not have clock_gettime, use clock_get_time
  clock_get_time(cclock, &mts);
  ticTime.tv_sec = mts.tv_sec;
  ticTime.tv_nsec = mts.tv_nsec;
#else
  if (clock_gettime(CLOCK_REALTIME, &ticTime) != 0) {
    ++errors;
    cerr << "ChronoCpu::doTic - " << name << ": clock_gettime() failed!"
         << endl;
    return;
  }
#endif

  ++ticCounter;

  if (ticCounter > 1) {
    float period_s = (float)(ticTime.tv_sec - lastTicTime.tv_sec);
    float period_ns = (float)(ticTime.tv_nsec - lastTicTime.tv_nsec);
    periodStats.lastTime_ms = period_s * 1e3f + period_ns / 1e6f;
    updateStats(periodStats);
  }
}

void ChronoCpu::doTac(void) {
#ifdef __MACH__ // OS X does not have clock_gettime, use clock_get_time
  clock_serv_t cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  tacTime.tv_sec = mts.tv_sec;
  tacTime.tv_nsec = mts.tv_nsec;

#else
  if (clock_gettime(CLOCK_REALTIME, &tacTime) != 0) {
    ++errors;
    cerr << "ChronoCpu::doTac - " << name << ": clock_gettime() failed!"
         << endl;
    return;
  }
#endif

  float elapsed_s = (float)(tacTime.tv_sec - ticTime.tv_sec);
  float elapsed_ns = (float)(tacTime.tv_nsec - ticTime.tv_nsec);
  elapsedStats.lastTime_ms = elapsed_s * 1e3f + elapsed_ns / 1e6f;
  updateStats(elapsedStats);
}
