/**
 * @file	socket.cc
 * @author	Joseph Lee <joseph@jc-lab.net>
 * @date	2021-09-12
 * @copyright Copyright (C) 2021 jc-lab. All rights reserved.
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */

#include <jcu-unio/net/socket.h>

#include <utility>

namespace jcu {
namespace unio {

SocketReadEvent::SocketReadEvent(Buffer *buffer) :
    AbstractEvent(nullptr), buffer_(buffer) {}

SocketReadEvent::SocketReadEvent(std::shared_ptr<ErrorEvent> error, Buffer *buffer) :
    AbstractEvent(std::move(error)), buffer_(buffer) {}

SocketWriteEvent::SocketWriteEvent() :
    AbstractEvent(nullptr) {}

SocketWriteEvent::SocketWriteEvent(std::shared_ptr<ErrorEvent> error) :
    AbstractEvent(std::move(error)) {}

} // namespace unio
} // namespace jcu
