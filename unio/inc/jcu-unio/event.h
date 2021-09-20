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
  virtual bool hasError() const = 0;
  virtual ErrorEvent& error() = 0;
  virtual const ErrorEvent& error() const = 0;
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
