/**
 * @file	unit_test_utils.cc
 * @author	Joseph Lee <joseph@jc-lab.net>
 * @date	2021-09-28
 * @copyright Copyright (C) 2021 jc-lab. All rights reserved.
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */

#include <jcu-unio/loop.h>
#include <jcu-unio/log.h>

#include "unit_test_utils.h"

namespace jcu {
namespace unio {

void LoopSupportTest::SetUp() {
  stopped_.store(false);
  basic_params_.logger = createDefaultLogger([](Logger::LogLevel level, const std::string& text) -> void {
    fprintf(stderr, "%s\n", text.c_str());
  });
  basic_params_.loop = SharedLoop::create();
  thread_ = std::thread([&]() -> void {
    fprintf(stderr, "LoopThread: start\n");
    basic_params_.loop->init();
    uv_run(basic_params_.loop->get(), UV_RUN_DEFAULT);
    stopped_.store(true);
    fprintf(stderr, "LoopThread: stopped\n");
  });
}

void LoopSupportTest::TearDown() {
  basic_params_.loop->sendQueuedTask([&]() -> void {
    basic_params_.loop->uninit();
  });
  for (int i = 0; (!stopped_.load()) && (i < 50); i++) {
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
  }
  EXPECT_TRUE(stopped_.load());
  if (stopped_.load()) {
    thread_.join();
  } else {
    thread_.detach();
  }
  EXPECT_EQ(basic_params_.loop.use_count(), 1);
}

} // namespace unio
} // namespace jcu
