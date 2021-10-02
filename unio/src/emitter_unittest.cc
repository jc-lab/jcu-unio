/**
 * @file	emitter_unittest.cc
 * @author	Joseph Lee <joseph@jc-lab.net>
 * @date	2021-09-28
 * @copyright Copyright (C) 2021 jc-lab. All rights reserved.
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */

#include <string>
#include <atomic>
#include <thread>
#include <future>

#include <gtest/gtest.h>

#include <jcu-unio/emitter.h>
#include <jcu-unio/handle.h>

namespace {

using namespace jcu::unio;

class EmitterTest : public ::testing::Test {
};

class TestObject : public Handle {
 public:
  std::weak_ptr<TestObject> self_;
  std::mutex init_mtx_;
  std::thread init_th_;

  TestObject() {
    init_th_ = std::thread([&]() -> void {
      std::this_thread::sleep_for(std::chrono::milliseconds { 500 });
      this->init();
    });
  }

  ~TestObject() {
    if (init_th_.joinable()) {
      init_th_.join();
    }
  }

  void close() override {
  }

  static std::shared_ptr<TestObject> create() {
    std::shared_ptr<TestObject> instance(new TestObject());
    instance->self_ = instance;
    return std::move(instance);
  }

 protected:
  std::shared_ptr<Resource> sharedAsResource() override {
    return self_.lock();
  }

  std::mutex &getInitMutex() override {
    return init_mtx_;
  }

  void _init() override {
    InitEvent event;
    emitInit(std::move(event));
  }
};

class AlphaEvent {
 public:
  virtual std::string name() const {
    return "AlphaEvent";
  }
};

class AppleEvent : public AlphaEvent {
 public:
  std::string name() const override {
    return "AppleEvent";
  }
};

TEST_F(EmitterTest, ExactEventOnce) {
  std::shared_ptr<TestObject> instance(TestObject::create());
  std::atomic_int result(0);
  std::shared_ptr<int> sobj_a(new int(1));
  std::shared_ptr<int> sobj_b(new int(2));
  std::shared_ptr<int> sobj_c(new int(3));
  std::shared_ptr<int> sobj_d(new int(4));

  instance->once<AlphaEvent>([&result, sobj_a](AlphaEvent& event, auto& resource) -> void {
    result.fetch_add(0x00000001);
  });
  instance->once<AlphaEvent>([&result, sobj_b](AlphaEvent& event, auto& resource) -> void {
    result.fetch_add(0x00000100);
  });
  instance->once<AppleEvent>([&result, sobj_c](AppleEvent& event, auto& resource) -> void {
    result.fetch_add(0x00010000);
  });
  instance->once<AppleEvent>([&result, sobj_d](AppleEvent& event, auto& resource) -> void {
    result.fetch_add(0x01000000);
  });

  {
    AlphaEvent event;
    instance->emit(event);
    instance->emit(event);
    instance->emit(event);
    instance->emit(event);
  }

  EXPECT_EQ(result.load(), 0x00000101);
  EXPECT_EQ(sobj_a.use_count(), 1);
  EXPECT_EQ(sobj_b.use_count(), 1);
  EXPECT_EQ(sobj_c.use_count(), 2);
  EXPECT_EQ(sobj_d.use_count(), 2);

  {
    AppleEvent event;
    instance->emit(event);
    instance->emit(event);
    instance->emit(event);
    instance->emit(event);
  }

  EXPECT_EQ(sobj_c.use_count(), 1);
  EXPECT_EQ(sobj_d.use_count(), 1);
}

TEST_F(EmitterTest, ExactEventOn) {
  std::shared_ptr<TestObject> instance(TestObject::create());
  std::atomic_int result(0);
  std::shared_ptr<int> sobj_a(new int(1));
  std::shared_ptr<int> sobj_b(new int(2));
  std::shared_ptr<int> sobj_c(new int(3));
  std::shared_ptr<int> sobj_d(new int(4));

  instance->on<AlphaEvent>([&result, sobj_a](AlphaEvent& event, auto& resource) -> void {
    result.fetch_add(0x00000001);
  });
  instance->on<AlphaEvent>([&result, sobj_b](AlphaEvent& event, auto& resource) -> void {
    result.fetch_add(0x00000100);
  });
  instance->on<AppleEvent>([&result, sobj_c](AppleEvent& event, auto& resource) -> void {
    result.fetch_add(0x00010000);
  });
  instance->on<AppleEvent>([&result, sobj_d](AppleEvent& event, auto& resource) -> void {
    result.fetch_add(0x01000000);
  });

  {
    AlphaEvent event;
    instance->emit(event);
    instance->emit(event);
    instance->emit(event);
    instance->emit(event);
  }

  EXPECT_EQ(result.load(), 0x00000404);
  EXPECT_EQ(sobj_a.use_count(), 2);
  EXPECT_EQ(sobj_b.use_count(), 2);
  EXPECT_EQ(sobj_c.use_count(), 2);
  EXPECT_EQ(sobj_d.use_count(), 2);

  result.store(0);
  {
    AppleEvent event;
    instance->emit(event);
    instance->emit(event);
    instance->emit(event);
    instance->emit(event);
  }

  EXPECT_EQ(result.load(), 0x04040000);
  EXPECT_EQ(sobj_a.use_count(), 2);
  EXPECT_EQ(sobj_b.use_count(), 2);
  EXPECT_EQ(sobj_c.use_count(), 2);
  EXPECT_EQ(sobj_d.use_count(), 2);

  instance->offAll();
  EXPECT_EQ(sobj_a.use_count(), 1);
  EXPECT_EQ(sobj_b.use_count(), 1);
  EXPECT_EQ(sobj_c.use_count(), 1);
  EXPECT_EQ(sobj_d.use_count(), 1);
}

TEST_F(EmitterTest, InheritedEventOnce) {
  std::shared_ptr<TestObject> instance(TestObject::create());
  std::atomic_int result(0);
  std::shared_ptr<int> sobj_a(new int(1));
  std::shared_ptr<int> sobj_b(new int(2));
  std::shared_ptr<int> sobj_c(new int(3));
  std::shared_ptr<int> sobj_d(new int(4));

  instance->once<AlphaEvent>([&result, sobj_a](AlphaEvent& event, auto& resource) -> void {
    result.fetch_add(0x00000001);
  });
  instance->once<AlphaEvent>([&result, sobj_b](AlphaEvent& event, auto& resource) -> void {
    result.fetch_add(0x00000100);
  });
  instance->once<AppleEvent>([&result, sobj_c](AppleEvent& event, auto& resource) -> void {
    result.fetch_add(0x00010000);
  });
  instance->once<AppleEvent>([&result, sobj_d](AppleEvent& event, auto& resource) -> void {
    result.fetch_add(0x01000000);
  });

  {
    AppleEvent event;
    instance->emit<AlphaEvent>(event);
    instance->emit<AlphaEvent>(event);
    instance->emit<AlphaEvent>(event);
    instance->emit<AlphaEvent>(event);
  }

  EXPECT_EQ(result.load(), 0x00000101);
  EXPECT_EQ(sobj_a.use_count(), 1);
  EXPECT_EQ(sobj_b.use_count(), 1);
  EXPECT_EQ(sobj_c.use_count(), 2);
  EXPECT_EQ(sobj_d.use_count(), 2);

  instance->off<AppleEvent>();

  EXPECT_EQ(sobj_a.use_count(), 1);
  EXPECT_EQ(sobj_b.use_count(), 1);
  EXPECT_EQ(sobj_c.use_count(), 1);
  EXPECT_EQ(sobj_d.use_count(), 1);
}

TEST_F(EmitterTest, InheritedEventOn) {
  std::shared_ptr<TestObject> instance(TestObject::create());
  std::atomic_int result(0);
  std::shared_ptr<int> sobj_a(new int(1));
  std::shared_ptr<int> sobj_b(new int(2));
  std::shared_ptr<int> sobj_c(new int(3));
  std::shared_ptr<int> sobj_d(new int(4));

  instance->on<AlphaEvent>([&result, sobj_a](AlphaEvent& event, auto& resource) -> void {
    result.fetch_add(0x00000001);
  });
  instance->on<AlphaEvent>([&result, sobj_b](AlphaEvent& event, auto& resource) -> void {
    result.fetch_add(0x00000100);
  });
  instance->on<AppleEvent>([&result, sobj_c](AppleEvent& event, auto& resource) -> void {
    result.fetch_add(0x00010000);
  });
  instance->on<AppleEvent>([&result, sobj_d](AppleEvent& event, auto& resource) -> void {
    result.fetch_add(0x01000000);
  });

  {
    AppleEvent event;
    instance->emit<AlphaEvent>(event);
    instance->emit<AlphaEvent>(event);
    instance->emit<AlphaEvent>(event);
    instance->emit<AlphaEvent>(event);
  }

  EXPECT_EQ(result.load(), 0x00000404);
  EXPECT_EQ(sobj_a.use_count(), 2);
  EXPECT_EQ(sobj_b.use_count(), 2);
  EXPECT_EQ(sobj_c.use_count(), 2);
  EXPECT_EQ(sobj_d.use_count(), 2);

  instance->offAll();
  EXPECT_EQ(sobj_a.use_count(), 1);
  EXPECT_EQ(sobj_b.use_count(), 1);
  EXPECT_EQ(sobj_c.use_count(), 1);
  EXPECT_EQ(sobj_d.use_count(), 1);
}

TEST_F(EmitterTest, AutoInitTestOnce) {
  std::shared_ptr<TestObject> instance(TestObject::create());
  std::atomic_int result(0);
  std::promise<int> p;
  std::future<int> future = p.get_future();

  instance->once<jcu::unio::InitEvent>([&](auto& event, auto& resource) -> void {
    result.fetch_add(0x00000001);
    p.set_value(1);
  });

  auto wait_result = future.wait_for(std::chrono::milliseconds { 1000 });
  EXPECT_EQ(wait_result, std::future_status::ready);
  EXPECT_EQ(result, 0x00000001);
}

TEST_F(EmitterTest, AutoInitTestOn) {
  std::shared_ptr<TestObject> instance(TestObject::create());
  std::atomic_int result(0);
  std::promise<int> p;
  std::future<int> future = p.get_future();

  instance->once<jcu::unio::InitEvent>([&](auto& event, auto& resource) -> void {
    result.fetch_add(0x00000001);
    p.set_value(1);
  });

  auto wait_result = future.wait_for(std::chrono::milliseconds { 1000 });
  EXPECT_EQ(wait_result, std::future_status::ready);
  EXPECT_EQ(result, 0x00000001);
}

TEST_F(EmitterTest, ManuallyInitTestOnce) {
  std::shared_ptr<TestObject> instance(TestObject::create());
  std::atomic_int result(0);

  // Order is importance!
  instance->init();
  instance->once<jcu::unio::InitEvent>([&](auto& event, auto& resource) -> void {
    result.fetch_add(0x00000001);
  });

  EXPECT_EQ(result, 0x00000001);
}

TEST_F(EmitterTest, ManuallyInitTestOn) {
  std::shared_ptr<TestObject> instance(TestObject::create());
  std::atomic_int result(0);

  // Order is importance!
  instance->init();
  instance->once<jcu::unio::InitEvent>([&](auto& event, auto& resource) -> void {
    result.fetch_add(0x00000001);
  });

  EXPECT_EQ(result, 0x00000001);
}

}
