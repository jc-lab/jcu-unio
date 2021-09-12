/**
 * @file	socket.cc
 * @author	Joseph Lee <joseph@jc-lab.net>
 * @date	2021-09-12
 * @copyright Copyright (C) 2021 jc-lab. All rights reserved.
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */

#include <jcu-unio/net/socket.h>

namespace jcu {
namespace unio {

SocketReadEvent::SocketReadEvent(Buffer *buffer) :
    error_({}), buffer_(buffer) {}

SocketReadEvent::SocketReadEvent(UvErrorEvent error, Buffer *buffer) :
    error_(error), buffer_(buffer) {}

bool SocketReadEvent::hasError() const {
  return error_.code() != 0;
}

ErrorEvent &SocketReadEvent::error() {
  return error_;
}

const ErrorEvent &SocketReadEvent::error() const {
  return error_;
}

SocketWriteEvent::SocketWriteEvent() :
    error_({}) {}

SocketWriteEvent::SocketWriteEvent(UvErrorEvent error) :
    error_(error) {}

bool SocketWriteEvent::hasError() const {
  return error_.code() != 0;
}

ErrorEvent &SocketWriteEvent::error() {
  return error_;
}

const ErrorEvent &SocketWriteEvent::error() const {
  return error_;
}

} // namespace unio
} // namespace jcu
