/**
 * @file	unio.cc
 * @author	Joseph Lee <joseph@jc-lab.net>
 * @date	2021-08-28
 * @copyright Copyright (C) 2021 jc-lab. All rights reserved.
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */

#include <memory>
#include <cstdlib>
#include <mutex>
#include <deque>

#include <jcu-unio/loop.h>
#include <jcu-unio/shared_object.h>

namespace jcu {
namespace unio {

struct QueuedTaskResource {
  QueuedTask_t task;
};

struct LoopContext {
  uv_async_t queue_handle;
  std::mutex mutex;
  std::deque<QueuedTaskResource> queue;

  void processQueuedTask() {
    std::unique_lock<std::mutex> lock(mutex);
    while (!queue.empty()) {
      QueuedTaskResource resource = std::move(queue.front());
      queue.pop_front();
      resource.task();
    }
  }
};

Loop::Loop() {
  ctx_ = std::make_unique<LoopContext>();
  std::memset(&ctx_->queue_handle, 0, sizeof(ctx_->queue_handle));
}

void Loop::init() {
  uv_async_init(get(), &ctx_->queue_handle, [](uv_async_t* handle) -> void {
    Loop* self = (Loop*) uv_handle_get_data((uv_handle_t*) handle);
    self->ctx_->processQueuedTask();
  });
  uv_handle_set_data((uv_handle_t*)&ctx_->queue_handle, this);
  uv_async_send(&ctx_->queue_handle);
}

void Loop::uninit() {
  uv_close((uv_handle_t*)&ctx_->queue_handle, [](uv_handle_t* handle) -> void {});
}

void Loop::sendQueuedTask(QueuedTask_t&& task) const {
  std::unique_lock<std::mutex> lock(ctx_->mutex);
  QueuedTaskResource resource { task };
  ctx_->queue.emplace_back(std::move(resource));
  if (ctx_->queue_handle.type) {
    int rc = uv_async_send(&ctx_->queue_handle);
  }
}

class UnsafeLoopImpl : public UnsafeLoop {
 private:
  uv_loop_t *ptr_;
  std::weak_ptr<UnsafeLoopImpl> self_;

 public:
  static std::shared_ptr<UnsafeLoopImpl> create(uv_loop_t* ptr) {
    std::shared_ptr<UnsafeLoopImpl> instance(std::make_shared<UnsafeLoopImpl>(ptr));
    instance->self_ = instance;
    return std::move(instance);
  }

  UnsafeLoopImpl(uv_loop_t* ptr) :
      ptr_(ptr)
  {}

  std::shared_ptr<Loop> shared() const override {
    return self_.lock();
  }

  uv_loop_t *get() const override {
    return ptr_;
  }
};

std::shared_ptr<UnsafeLoop> UnsafeLoop::create(uv_loop_t *loop) {
  return UnsafeLoopImpl::create(loop);
}

std::shared_ptr<UnsafeLoop> UnsafeLoop::fromDefault() {
  return UnsafeLoop::create(uv_default_loop());
}

class SharedLoopImpl : public SharedLoop {
 private:
  uv_loop_t *ptr_;
  std::weak_ptr<SharedLoopImpl> self_;

 public:
  static std::shared_ptr<SharedLoopImpl> create() {
    std::shared_ptr<SharedLoopImpl> instance(std::make_shared<SharedLoopImpl>());
    instance->self_ = instance;
    return std::move(instance);
  }

  SharedLoopImpl() {
    ptr_ = uv_loop_new();
  }

  ~SharedLoopImpl() override {
    if (ptr_) {
      uv_loop_delete(ptr_);
      ptr_ = nullptr;
    }
  }

  std::shared_ptr<Loop> shared() const override {
    return self_.lock();
  }

  uv_loop_t *get() const override {
    return ptr_;
  }
};

std::shared_ptr<SharedLoop> SharedLoop::create() {
  return SharedLoopImpl::create();
}

} // namespace unio
} // namespace jcu

