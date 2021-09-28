/**
 * @file	tcp_socket_unittest.cc
 * @author	Joseph Lee <joseph@jc-lab.net>
 * @date	2021-09-28
 * @copyright Copyright (C) 2021 jc-lab. All rights reserved.
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */

#include "../test/unit_test_utils.h"
#include <jcu-unio/loop.h>

#include <jcu-unio/net/tcp_socket.h>

namespace {

using namespace jcu::unio;

class TcpSocketTest : public LoopSupportTest {
};

TEST_F(TcpSocketTest, TcpSocketTestLifecycle) {
  std::promise<int> p;
  std::future<int> f = p.get_future();
  std::weak_ptr<TCPSocket> weak_handle;

  do {
    auto handle = TCPSocket::create(basic_params_);
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

}
