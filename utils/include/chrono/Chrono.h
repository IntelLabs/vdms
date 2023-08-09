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

#ifndef CHRONO_H_
#define CHRONO_H_

#include <iostream>
#include <stdint.h>
#include <string.h>
#include <string>

#include <time.h>

// Apple OSX
#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#ifdef CHRONO_TIMING
#define CHRONO_TIC(NAME) NAME.tic();
#define CHRONO_TAC(NAME) NAME.tac();
#define CHRONO_PRINT_LAST_MS(NAME) NAME.printLastTime_ms();
#else
#define CHRONO_TIC(NAME)
#define CHRONO_TAC(NAME)
#define CHRONO_PRINT_LAST_MS(NAME)
#endif

// ***************************************************************************
// Chrono Base class
// ***************************************************************************
class Chrono {
public:
  Chrono(const std::string &name, const bool asyncEnabled = false);
  Chrono();
  virtual ~Chrono(void);

  void tic(void);
  void tac(void);
  void reset(void);
  void setEnabled(const bool val);

  struct ChronoStats {
    std::string name;
    uint32_t counter;
    float totalTime_ms;
    float totalSquaredTime_ms2;
    float averageTime_ms;
    float stdDevTime_ms;
    float lastTime_ms;
    float minTime_ms;
    float maxTime_ms;
  };

  const Chrono::ChronoStats &getElapsedStats(void) const {
    return elapsedStats;
  }

  const Chrono::ChronoStats &getPeriodStats(void) const { return periodStats; }

  uint32_t getTotalTime_ms(void) const { return elapsedStats.totalTime_ms; }

  uint32_t getTotalTime_us(void) const {
    return elapsedStats.totalTime_ms * 1000.0f;
  }

  uint32_t getLastTime_ms(void) const { return elapsedStats.lastTime_ms; }

  uint32_t getLastTime_us(void) const {
    return elapsedStats.lastTime_ms * 1000.0f;
  }

  uint32_t getAvgTime_ms(void) const { return elapsedStats.averageTime_ms; }

  uint32_t getAvgTime_us(void) const {
    return elapsedStats.averageTime_ms * 1000.0f;
  }

  uint32_t getSTD_ms(void) const { return elapsedStats.stdDevTime_ms; }

  uint32_t getSTD_us(void) const {
    return elapsedStats.stdDevTime_ms * 1000.0f;
  }

  void printTotalTime_ms(void) const {
    std::cout << name << ": " << getTotalTime_ms() << " [ms]" << std::endl;
  }

  void printTotalTime_us(void) const {
    std::cout << name << ": " << getTotalTime_us() << " [us]" << std::endl;
  }

  void printLastTime_ms(void) const {
    std::cout << name << ": " << getLastTime_ms() << " [ms]" << std::endl;
  }

  void printLastTime_us(void) const {
    std::cout << name << ": " << getLastTime_us() << " [us]" << std::endl;
  }

  void printAvgTime_ms(void) const {
    std::cout << name << ": " << getAvgTime_ms() << " [ms]" << std::endl;
  }

  void printAvgTime_us(void) const {
    std::cout << name << ": " << getAvgTime_us() << " [us]" << std::endl;
  }

  std::ostream &printStats(const Chrono::ChronoStats &stats,
                           std::ostream &os) const;
  std::ostream &printAvgTime(const Chrono::ChronoStats &stats,
                             std::ostream &os) const;
  std::ostream &printAvgTime(const Chrono::ChronoStats &stats, std::ostream &os,
                             const float ref) const;

protected:
  std::string name;

  bool enabled;
  bool ticIdle;
  uint32_t errors;

  ChronoStats elapsedStats;
  ChronoStats periodStats;

  void resetStats(ChronoStats &stats);
  void updateStats(ChronoStats &stats);

  virtual void doTic(void) = 0;
  virtual void doTac(void) = 0;
};

// ***************************************************************************
// Chrono Cpu Implementation
// ***************************************************************************

class ChronoCpu : public Chrono {
public:
  ChronoCpu(const std::string &name);
  ChronoCpu();
  ~ChronoCpu(void);

protected:
  timespec lastTicTime;
  timespec ticTime;
  timespec tacTime;

#ifdef __MACH__
  clock_serv_t cclock;
  mach_timespec_t mts;
#endif

  uint32_t ticCounter;

  virtual void doTic(void);
  virtual void doTac(void);
};

#endif // CHRONO_H_
