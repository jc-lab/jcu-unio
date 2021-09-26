/**
 * @file	event.h
 * @author	Joseph Lee <joseph@jc-lab.net>
 * @date	2021-09-12
 * @copyright Copyright (C) 2021 jc-lab. All rights reserved.
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */

#ifndef JCU_UNIO_EVENT_H_
#define JCU_UNIO_EVENT_H_

#include <memory>
#include <functional>
#include <string>

#include "resource.h"

namespace jcu {
namespace unio {

class ErrorEvent {
 public:
  virtual int code() const = 0;
  virtual const char* what() const = 0;
};

class CloseEvent {
};

class UvErrorEvent : public ErrorEvent {
 protected:
  int code_;
  int sys_error_;
  std::string what_;

 public:
  UvErrorEvent();
  UvErrorEvent(int uv_error, int sys_error);

  static std::shared_ptr<UvErrorEvent> createIfNeeded(int uv_error, int sys_error = 0) {
    if (uv_error || sys_error) {
      return std::make_shared<UvErrorEvent>(uv_error, sys_error);
    }
    return nullptr;
  }

  int code() const override {
    return code_;
  }
  int sys_error() const {
    return sys_error_;
  }
  const char *what() const override {
    return what_.c_str();
  }
};

class BaseEvent {
 public:
  virtual ~BaseEvent() = default;
  virtual bool hasError() const = 0;
  virtual ErrorEvent& error() = 0;
  virtual const ErrorEvent& error() const = 0;
};

class AbstractEvent : public BaseEvent {
 protected:
  std::shared_ptr<ErrorEvent> error_;

 public:
  AbstractEvent(std::shared_ptr<ErrorEvent> error);
  bool hasError() const override;
  ErrorEvent &error() override;
  const ErrorEvent &error() const override;
};

class InitEvent : public AbstractEvent {
 public:
  InitEvent();
  InitEvent(std::shared_ptr<ErrorEvent> error);
};

/**
 * @tparam T event type
 * @tparam B base handle type
 */
template <typename T>
using CompletionCallback = std::function<void(T& event, Resource& handle)>;

template <typename T>
using CompletionOnceCallback = CompletionCallback<T>;

template <typename T>
using CompletionManyCallback = CompletionCallback<T>;

} // namespace unio
} // namespace jcu

#endif //JCU_UNIO_EVENT_H_
