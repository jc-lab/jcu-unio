/**
 * @file	stream_socket.cc
 * @author	Joseph Lee <joseph@jc-lab.net>
 * @date	2021-09-12
 * @copyright Copyright (C) 2021 jc-lab. All rights reserved.
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */

#include <jcu-unio/net/stream_socket.h>

#include <utility>

namespace jcu {
namespace unio {

SocketConnectEvent::SocketConnectEvent() :
    AbstractEvent(nullptr) {}

SocketConnectEvent::SocketConnectEvent(std::shared_ptr<ErrorEvent> error) :
    AbstractEvent(std::move(error)) {}

SocketDisconnectEvent::SocketDisconnectEvent() :
    AbstractEvent(nullptr) {}

SocketDisconnectEvent::SocketDisconnectEvent(std::shared_ptr<ErrorEvent> error) :
    AbstractEvent(std::move(error)) {}

} // namespace unio
} // namespace jcu
