/**
 * @file	loop_unittest.cc
 * @author	Joseph Lee <joseph@jc-lab.net>
 * @date	2021-09-28
 * @copyright Copyright (C) 2021 jc-lab. All rights reserved.
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */

#include "../test/unit_test_utils.h"
#include <jcu-unio/loop.h>

namespace {

using namespace jcu::unio;

class LoopTest : public LoopSupportTest {
};

TEST_F(LoopTest, BasicSendTask) {
  std::atomic_int test_value;
  basic_params_.loop->sendQueuedTask([&test_value]() -> void {
    test_value.store(1);
  });
  for (int i=0; i<100; i++) {
    std::this_thread::sleep_for(std::chrono::milliseconds{10});
  }
  EXPECT_EQ(test_value.load(), 1);
}

}
