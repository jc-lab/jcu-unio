/**
 * @file	ssl_context.cc
 * @author	Joseph Lee <joseph@jc-lab.net>
 * @date	2021-09-19
 * @copyright Copyright (C) 2021 jc-lab. All rights reserved.
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */

#include <jcu-unio/net/ssl_context.h>

namespace jcu {
namespace unio {

HandshakeEvent::HandshakeEvent() : error_({}) {
}
HandshakeEvent::HandshakeEvent(SslErrorEvent error) : error_(std::move(error)) {
}
bool HandshakeEvent::hasError() const {
  return error_.code() != 0;
}
ErrorEvent &HandshakeEvent::error() {
  return error_;
}
const ErrorEvent &HandshakeEvent::error() const {
  return error_;
}

SslErrorEvent::SslErrorEvent() :
    code_(0) {}

SslErrorEvent::SslErrorEvent(int code, const std::string &message) :
    code_(code), message_(message) {
}

int SslErrorEvent::code() const {
  return code_;
}

const char *SslErrorEvent::what() const {
  return message_.c_str();
}
} // namespace unio
} // namespace jcu
