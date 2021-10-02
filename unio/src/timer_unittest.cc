/**
 * @file	timer_unittest.cc
 * @author	Joseph Lee <joseph@jc-lab.net>
 * @date	2021-09-28
 * @copyright Copyright (C) 2021 jc-lab. All rights reserved.
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */

#include "../test/unit_test_utils.h"
#include <jcu-unio/loop.h>

#include <jcu-unio/timer.h>

namespace {

using namespace jcu::unio;

class TimerTest : public LoopSupportTest {
};

TEST_F(TimerTest, TimerLifecycle) {
  std::promise<int> p;
  std::future<int> f = p.get_future();
  std::weak_ptr<Timer> weak_handle;

  do {
    auto handle = Timer::create(basic_params_);
    weak_handle = handle;
    handle->once<InitEvent>([&p](auto &event, auto &resource) mutable -> void {
      resource.close();
      p.set_value(1);
    });
  } while (0);

  EXPECT_EQ(f.wait_for(std::chrono::milliseconds{1000}), std::future_status::ready);
  std::this_thread::sleep_for(std::chrono::milliseconds{100}); // Wait for the callback to complete.
  EXPECT_EQ(weak_handle.use_count(), 0);
}

TEST_F(TimerTest, Timeout) {
  std::promise<int> init_promise;
  std::future<int> init_future = init_promise.get_future();

  std::promise<int> close_promise;
  std::future<int> close_future = close_promise.get_future();

  std::promise<int> timeout_promise;
  std::future<int> timeout_future = timeout_promise.get_future();

  std::weak_ptr<Timer> weak_handle;
  std::atomic_int result(0);

  do {
    auto handle = Timer::create(basic_params_);
    weak_handle = handle;
    handle->once<CloseEvent>([&](auto& event, auto& resource) -> void {
      close_promise.set_value(1);
    });
    handle->once<TimerEvent>([&](auto& event, auto& resource) -> void {
      if (result.fetch_add(1) == 0) {
        timeout_promise.set_value(1);
      }
    });
    handle->on<InitEvent>([&](auto &event, auto &resource) mutable -> void {
      auto& timer = dynamic_cast<Timer&>(resource);
      timer.start(std::chrono::milliseconds { 100 });
      init_promise.set_value(1);
    });
  } while (0);

  // waiting for InitEvent
  EXPECT_EQ(init_future.wait_for(std::chrono::milliseconds{1000}), std::future_status::ready);

  std::this_thread::sleep_for(std::chrono::milliseconds{1000});

  // waiting for InitEvent
  EXPECT_EQ(timeout_future.wait_for(std::chrono::milliseconds{1000}), std::future_status::ready);
  EXPECT_EQ(result.load(), 1);

  EXPECT_EQ(weak_handle.use_count(), 1);
  basic_params_.loop->sendQueuedTask([timer = weak_handle.lock()]() -> void {
    timer->close();
  });

  // waiting for CloseEvent
  EXPECT_EQ(close_future.wait_for(std::chrono::milliseconds{1000}), std::future_status::ready);
  std::this_thread::sleep_for(std::chrono::milliseconds{100}); // Wait for the callback to complete.
  EXPECT_EQ(weak_handle.use_count(), 0);
}

}
