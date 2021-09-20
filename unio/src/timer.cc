/**
 * @file	timer.cc
 * @author	Joseph Lee <joseph@jc-lab.net>
 * @date	2021-09-20
 * @copyright Copyright (C) 2021 jc-lab. All rights reserved.
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */

#include <jcu-unio/loop.h>
#include <jcu-unio/log.h>
#include <jcu-unio/timer.h>
#include <jcu-unio/uv_helper.h>

#include <thread>

namespace jcu {
namespace unio {

class TimerImpl : public Timer {
 public:
  class HandleRef : public UvRef<uv_timer_t, TimerImpl> {
   public:
    HandleRef() : UvRef(nullptr) {}
    void close() override {
      data_.reset();
    }
    void setData(std::shared_ptr<TimerImpl> data) {
      data_ = data;
    }
  };

  std::weak_ptr<TimerImpl> self_;

  std::shared_ptr<Loop> loop_;
  std::shared_ptr<Logger> log_;

  HandleRef handle_;

  TimerImpl(std::shared_ptr<Loop> loop, std::shared_ptr<Logger> logger) :
    loop_(loop), log_(logger)
  {
    log_->logf(Logger::kLogTrace, "Timer: construct");
  }

  ~TimerImpl()
  {
    log_->logf(Logger::kLogTrace, "Timer: destroy");
  }

  std::shared_ptr<Timer> shared() const override {
    return self_.lock();
  }

  static void closeCallback(uv_handle_t* handle) {
    auto* ref = HandleRef::from(handle);
    std::shared_ptr<TimerImpl> self = ref->data();
    ref->close();
    self->log_->logf(jcu::unio::Logger::kLogTrace, "TimerImpl: closeCallback");
    CloseEvent event {};
    self->emit(event);
  }

  void close() override {
    uv_close(handle_.handle<uv_handle_t>(), closeCallback);
  }

  UvErrorEvent stop() override {
    int rc = uv_timer_stop(handle_.handle());
    return UvErrorEvent { rc, 0 };
  }

  UvErrorEvent again() override {
    int rc = uv_timer_again(handle_.handle());
    return UvErrorEvent { rc, 0 };
  }

  void setRepeat(uint64_t repeat) override {
    uv_timer_set_repeat(handle_.handle(), repeat);
  }

  std::chrono::milliseconds getRepeat() override {
    uint64_t value = uv_timer_get_repeat(handle_.handle());
    return std::chrono::milliseconds { value };
  }

  std::chrono::milliseconds getDueIn() override {
    uint64_t value = uv_timer_get_due_in(handle_.handle());
    return std::chrono::milliseconds { value };
  }

  static void timerCallback(uv_timer_t* handle) {
    auto ref = HandleRef::from(handle);
    auto self = ref->data();
    TimerEvent event;
    self->emit(event);
  }

  UvErrorEvent start(uint64_t timeout, uint64_t repeat) override {
    int rc;
    rc = uv_timer_init(loop_->get(), handle_.handle());
    if (rc) {
      return UvErrorEvent { rc, 0 };
    }
    handle_.setData(self_.lock());
    handle_.attach();

    rc = uv_timer_start(handle_.handle(), timerCallback, timeout, repeat);
    if (rc) {
      return UvErrorEvent { rc, 0 };
    }

    return UvErrorEvent {};
  }
};

std::shared_ptr<Timer> jcu::unio::Timer::create(std::shared_ptr<Loop> loop, std::shared_ptr<Logger> log) {
  std::shared_ptr<TimerImpl> instance(new TimerImpl(loop, log));
  instance->self_ = instance;
  return instance;
}

} // namespace unio
} // namespace jcu
