/**
 * @file	uv_helper.h
 * @author	Joseph Lee <joseph@jc-lab.net>
 * @date	2021-09-12
 * @copyright Copyright (C) 2021 jc-lab. All rights reserved.
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */

#ifndef JCU_UNIO_UV_HELPER_H_
#define JCU_UNIO_UV_HELPER_H_

#include <cstring>

#include "loop.h"
#include "event.h"

namespace jcu {
namespace unio {

class UvRefBase {
 public:
  virtual ~UvRefBase() = default;
  virtual uv_handle_t* baseHandle() const = 0;
  void attach() {
    uv_handle_set_data(baseHandle(), this);
  }
  virtual void close() {
    delete this;
  }
};

/**
 *
 * @tparam H uv handle type
 * @tparam D reference data type
 * @tparam F callback
 */
template <typename H, class D, typename F = std::function<void()>>
class UvRef : public UvRefBase {
 protected:
  H handle_;
  std::shared_ptr<D> data_;
  F fn_;

 public:
  UvRef(std::shared_ptr<D> data) :
      data_(data), fn_({})
  {
    std::memset(&handle_, 0, sizeof(handle_));
  }

  UvRef(std::shared_ptr<D> data, F fn) :
      data_(data), fn_(std::move(fn))
  {
    std::memset(&handle_, 0, sizeof(handle_));
  }

  uv_handle_t *baseHandle() const override {
    return (uv_handle_t*)&handle_;
  }

  H* handle() {
    return &handle_;
  }

  template <typename T>
  T* handle() {
    return (T*) &handle_;
  }

  std::shared_ptr<D> data() const {
    return data_;
  }

  void fn(F&& fn) {
    fn_ = std::move(fn);
  }

  template<typename... Args>
  auto invoke(Args&&... args) {
    return std::forward<F>(fn_)(std::forward<Args>(args)...);
  }

  static UvRef<H, D>* create(std::shared_ptr<D> data) {
    return new UvRef<H, D>(data);
  }

  static UvRef<H, D, F>* create(std::shared_ptr<D> data, F fn) {
    return new UvRef<H, D, F>(data, std::move(fn));
  }

  static UvRef<H, D, F>* from(void* handle) {
    UvRefBase* base = (UvRefBase*) uv_handle_get_data((uv_handle_t*)handle);
    return dynamic_cast<UvRef<H, D, F>*>(base);
  }
};

/**
 *
 * @tparam H uv handle type
 * @tparam D reference data type
 * @tparam F callback
 */
template <typename H, typename E, class D>
class UvCallbackRef : public UvRefBase {
 protected:
  H handle_;
  std::shared_ptr<D> data_;
  CompletionCallback<E> fn_;

 public:
  UvCallbackRef(std::shared_ptr<D> data) :
      data_(data), fn_(nullptr)
  {
    std::memset(&handle_, 0, sizeof(handle_));
  }

  UvCallbackRef(std::shared_ptr<D> data, CompletionCallback<E> fn) :
      data_(data), fn_(std::move(fn))
  {
    std::memset(&handle_, 0, sizeof(handle_));
  }

  uv_handle_t *baseHandle() const override {
    return (uv_handle_t*)&handle_;
  }

  H* handle() {
    return &handle_;
  }

  template <typename T>
  T* handle() {
    return (T*) &handle_;
  }

  std::shared_ptr<D> data() const {
    return data_;
  }

  void setCallback(CompletionCallback<E> fn) {
    fn_ = std::move(fn);
  }

  void publish(E event) {
    if (fn_) {
      fn_(event, *data_);
    } else {
      if (event.hasError()) {
        data_->template emit<ErrorEvent>(event.error());
      } else {
        data_->template emit<E>(event);
      }
    }
  }

  void publishAndClose(E event) {
    publish(std::move(event));
    close();
  }

  template<typename F, typename ...Args>
  bool reset(CompletionCallback<E> fn, F&& f, Args&& ...args) {
    int rc = std::forward<F>(f)(handle(), std::forward<Args>(args)...);
    if (rc) {
      E event { UvErrorEvent::createIfNeeded(rc, 0) };
      if (fn) {
        fn(event, *data_);
      } else {
        data_->template emit<ErrorEvent>(event.error());
      }
      return false;
    }
    setCallback(std::move(fn));
    attach();
    return true;
  }

  template<typename F, typename ...Args>
  static UvCallbackRef* create(std::shared_ptr<D> data, CompletionCallback<E> fn, F&& f, Args&& ...args) {
    auto* instance = new UvCallbackRef<H, E, D>(data);
    int rc = std::forward<F>(f)(instance->handle(), std::forward<Args>(args)...);
    if (rc) {
      delete instance;
      E event { UvErrorEvent::createIfNeeded(rc, 0) };
      if (fn) {
        fn(event, *data);
      } else {
        data->template emit<ErrorEvent>(event.error());
      }
      return nullptr;
    }
    instance->setCallback(std::move(fn));
    instance->attach();
    return instance;
  }

  static UvCallbackRef* from(void* handle) {
    UvRefBase* base = (UvRefBase*) uv_handle_get_data((uv_handle_t*)handle);
    return dynamic_cast<UvCallbackRef*>(base);
  }
};

} // namespace unio
} // namespace jcu

#endif //JCU_UNIO_UV_HELPER_H_
