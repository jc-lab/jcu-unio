/**
 * @file	tcp_socket_unittest.cc
 * @author	Joseph Lee <joseph@jc-lab.net>
 * @date	2021-09-28
 * @copyright Copyright (C) 2021 jc-lab. All rights reserved.
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */

#include <future>
#include <list>

#include "../test/unit_test_utils.h"
#include <jcu-unio/loop.h>

#include <jcu-unio/net/tcp_socket.h>

namespace {

using namespace jcu::unio;

class TcpSocketTest : public LoopSupportTest {
 public:
};

TEST_F(TcpSocketTest, ConnectSuccess) {
  std::list<int> ran_order;

  std::promise<int> p;
  std::future<int> f = p.get_future();

  auto server = TCPSocket::create(basic_params_); // ran_order: 1
  auto client = TCPSocket::create(basic_params_); // ran_order: 2

  const std::string address = "127.99.88.77";
  const unsigned int port = 65432;

  server->on<ErrorEvent>([](auto& event, auto& resource) -> void {
    fprintf(stderr, "server:ErrorEvent: %d, %s\n", event.code(), event.what());
    FAIL();
  });
  client->on<ErrorEvent>([](auto& event, auto& resource) -> void {
    fprintf(stderr, "client:ErrorEvent: %d, %s\n", event.code(), event.what());
    FAIL();
  });
  server->on<CloseEvent>([&](auto& event, auto& resource) -> void {
    fprintf(stderr, "server:CloseEvent\n");
    p.set_value(1);
  });
  client->on<CloseEvent>([](auto& event, auto& resource) -> void {
    fprintf(stderr, "client:CloseEvent\n");
  });

  server->once<SocketListenEvent>([&](auto& event, auto& resource) -> void {
    auto& server = dynamic_cast<TCPSocket&>(resource);
    fprintf(stderr, "SocketListenEvent\n");

    ran_order.push_back(3);

    auto socket = TCPSocket::create(basic_params_);
    socket->init();
    socket->on<ErrorEvent>([](auto& event, auto& resource) -> void {
      fprintf(stderr, "socket:ErrorEvent\n");
      FAIL();
    });
    socket->on<CloseEvent>([&server](auto& event, auto& resource) -> void {
      fprintf(stderr, "socket:CloseEvent\n");
      server.close();
    });
    socket->on<SocketEndEvent>([](auto& event, auto& resource) -> void {
      auto& handle = dynamic_cast<TCPSocket&>(resource);
      fprintf(stderr, "socket:SocketEndEvent\n");
      handle.close();
    });
    server.accept(socket);
    socket->read(createFixedSizeBuffer(1024));
  });
  client->once<SocketDisconnectEvent>([&](auto& event, auto& resource) -> void {
    auto& handle = dynamic_cast<TCPSocket&>(resource);
    fprintf(stderr, "client:SocketEndEvent\n");
    handle.close();
  });
  client->once<SocketConnectEvent>([&](auto& event, auto& resource) -> void {
    auto& handle = dynamic_cast<TCPSocket&>(resource);
    fprintf(stderr, "client:SocketConnectEvent\n");
    ran_order.push_back(3);
    handle.disconnect();
  });

  server->once<InitEvent>([&](auto& event, auto& resource) -> void {
    auto& handle = dynamic_cast<TCPSocket&>(resource);

    ran_order.push_back(1);

    auto bind_param = std::make_shared<SockAddrBindParam<sockaddr_in>>();
    EXPECT_EQ(uv_ip4_addr(address.c_str(), port, bind_param->getSockAddr()), 0);
    EXPECT_EQ(handle.bind(bind_param), 0);
    EXPECT_EQ(handle.listen(10), 0);
  });
  client->once<InitEvent>([&](auto& event, auto& resource) -> void {
    auto& handle = dynamic_cast<TCPSocket&>(resource);

    ran_order.push_back(2);

    auto connect_param = std::make_shared<SockAddrConnectParam<sockaddr_in>>();
    EXPECT_EQ(uv_ip4_addr(address.c_str(), port, connect_param->getSockAddr()), 0);
    handle.connect(connect_param);
  });

  EXPECT_EQ(f.wait_for(std::chrono::milliseconds { 50000 }), std::future_status::ready);
  std::this_thread::sleep_for(std::chrono::milliseconds { 100 }); // wait for the callback complete

  std::list<int> expect_ran_order {1, 2, 3, 3};
  EXPECT_EQ(ran_order, expect_ran_order);

  EXPECT_EQ(server.use_count(), 1);
  EXPECT_EQ(client.use_count(), 1);
}

TEST_F(TcpSocketTest, ConnectFailure) {
  std::promise<int> p_res;
  std::future<int> f_res = p_res.get_future();
  std::promise<int> p_close;
  std::future<int> f_close = p_close.get_future();

  auto client = TCPSocket::create(basic_params_); // ran_order: 2

  const std::string address = "255.255.255.255";
  const unsigned int port = 65432 + 1;

  client->on<ErrorEvent>([&](auto& event, auto& resource) -> void {
    fprintf(stderr, "client:ErrorEvent: %d, %s\n", event.code(), event.what());
    p_res.set_value(2);
    resource.close();
  });
  client->on<CloseEvent>([&](auto& event, auto& resource) -> void {
    fprintf(stderr, "client:CloseEvent\n");
    p_close.set_value(1);
  });
  client->once<SocketDisconnectEvent>([&](auto& event, auto& resource) -> void {
    auto& handle = dynamic_cast<TCPSocket&>(resource);
    fprintf(stderr, "client:SocketEndEvent\n");
    handle.close();
  });
  client->once<SocketConnectEvent>([&](auto& event, auto& resource) -> void {
    auto& handle = dynamic_cast<TCPSocket&>(resource);
    fprintf(stderr, "client:SocketConnectEvent\n");
    p_res.set_value(1);
    handle.close();
  });

  client->once<InitEvent>([&](auto& event, auto& resource) -> void {
    auto& handle = dynamic_cast<TCPSocket&>(resource);
    auto connect_param = std::make_shared<SockAddrConnectParam<sockaddr_in>>();
    EXPECT_EQ(uv_ip4_addr(address.c_str(), port, connect_param->getSockAddr()), 0);
    handle.connect(connect_param);
  });

  EXPECT_EQ(f_res.wait_for(std::chrono::milliseconds { 5000 }), std::future_status::ready);
  if (f_res.valid()) {
    EXPECT_EQ(f_res.get(), 2);
  }

  EXPECT_EQ(f_close.wait_for(std::chrono::milliseconds { 1000 }), std::future_status::ready);
  std::this_thread::sleep_for(std::chrono::milliseconds { 100 }); // wait for the callback complete

  EXPECT_EQ(client.use_count(), 1);
}

}
