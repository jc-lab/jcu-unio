/**
 * @file	stream_socket.cc
 * @author	Joseph Lee <joseph@jc-lab.net>
 * @date	2021-09-12
 * @copyright Copyright (C) 2021 jc-lab. All rights reserved.
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */

#include <jcu-unio/net/stream_socket.h>

namespace jcu {
namespace unio {

SocketConnectEvent::SocketConnectEvent() :
    error_({}) {}

SocketConnectEvent::SocketConnectEvent(UvErrorEvent error) :
    error_(error) {}

bool SocketConnectEvent::hasError() const {
  return error_.code() != 0;
}

ErrorEvent &SocketConnectEvent::error() {
  return error_;
}

const ErrorEvent &SocketConnectEvent::error() const {
  return error_;
}

SocketDisconnectEvent::SocketDisconnectEvent() :
    error_({}) {}

SocketDisconnectEvent::SocketDisconnectEvent(UvErrorEvent error) :
    error_(error) {}

bool SocketDisconnectEvent::hasError() const {
  return error_.code() != 0;
}

ErrorEvent &SocketDisconnectEvent::error() {
  return error_;
}

const ErrorEvent &SocketDisconnectEvent::error() const {
  return error_;
}

} // namespace unio
} // namespace jcu
