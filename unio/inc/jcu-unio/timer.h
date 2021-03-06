/**
 * @file	timer.h
 * @author	Joseph Lee <joseph@jc-lab.net>
 * @date	2021-09-20
 * @copyright Copyright (C) 2021 jc-lab. All rights reserved.
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */

#ifndef JCU_UNIO_TIMER_H_
#define JCU_UNIO_TIMER_H_

#include <chrono>

#include "handle.h"
#include "shared_object.h"
#include "event.h"

namespace jcu {
namespace unio {

class Loop;
class Logger;

class TimerEvent {};

class Timer : public Handle, public SharedObject<Timer> {
 public:
  static std::shared_ptr<Timer> create(const BasicParams& basic_params);

  template<class _RepTimout, class _PeriodTimeout, class _RepRepeat, class _PeriodRepeat>
  int start(
      const std::chrono::duration<_RepTimout, _PeriodTimeout> &timeout,
      const std::chrono::duration<_RepRepeat, _PeriodRepeat> &repeat
  ) {
    return start(
        std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count(),
        std::chrono::duration_cast<std::chrono::milliseconds>(repeat).count()
    );
  }

  template<class _RepTimout, class _PeriodTimeout>
  int start(
      const std::chrono::duration<_RepTimout, _PeriodTimeout> &timeout
  ) {
    return start(
        std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count(),
        0
    );
  }

  virtual int stop() = 0;
  virtual int again() = 0;

  template<class _RepRepeat, class _PeriodRepeat>
  void setRepeat(const std::chrono::duration<_RepRepeat, _PeriodRepeat> &repeat) {
    setRepeat(
        std::chrono::duration_cast<std::chrono::milliseconds>(repeat).count()
    );
  }

  virtual std::chrono::milliseconds getRepeat() = 0;
  virtual std::chrono::milliseconds getDueIn() = 0;

 protected:
  virtual int start(uint64_t timeout, uint64_t repeat) = 0;
  virtual void setRepeat(uint64_t repeat) = 0;
};

} // namespace unio
} // namespace jcu

#endif //JCU_UNIO_TIMER_H_
