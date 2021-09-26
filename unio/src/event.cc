/**
 * @file	event.cc
 * @author	Joseph Lee <joseph@jc-lab.net>
 * @date	2021-09-12
 * @copyright Copyright (C) 2021 jc-lab. All rights reserved.
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */

#include <uv.h>

#include <jcu-unio/event.h>

namespace jcu {
namespace unio {

UvErrorEvent::UvErrorEvent()
    : code_(0), sys_error_(0)
{
}

UvErrorEvent::UvErrorEvent(int uv_error, int sys_error)
    : code_(uv_error), sys_error_(sys_error)
{
  char buffer[256];
  if (uv_error == 0 && sys_error != 0) {
    uv_error = uv_translate_sys_error(sys_error);
  }
  const char* text = uv_err_name_r(uv_error, buffer, sizeof(buffer));
  if (text) {
    what_ = text;
  }
}

AbstractEvent::AbstractEvent(std::shared_ptr<ErrorEvent> error) :
  error_(std::move(error))
{}

bool AbstractEvent::hasError() const {
  return error_ != nullptr;
}

ErrorEvent &AbstractEvent::error() {
  return *error_;
}

const ErrorEvent &AbstractEvent::error() const {
  return *error_;
}

InitEvent::InitEvent() :
  AbstractEvent(nullptr) {}
InitEvent::InitEvent(std::shared_ptr<ErrorEvent> error) :
  AbstractEvent(error) {}

} // namespace unio
} // namespace jcu
