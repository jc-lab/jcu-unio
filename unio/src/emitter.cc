/**
 * @file	emitter.cc
 * @author	Joseph Lee <joseph@jc-lab.net>
 * @date	2021-09-26
 * @copyright Copyright (C) 2021 jc-lab. All rights reserved.
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */

#include <jcu-unio/emitter.h>

namespace jcu {
namespace unio {

template <>
void Emitter::on<InitEvent>(std::function<void(InitEvent& event, Resource& handle)> callback) {
  if (inited_) {
    callback(init_event_, dynamic_cast<Resource&>(*this));
    return ;
  }
  auto q = getHandler<InitEvent>();
  q->on(std::move(callback));
}

template <>
void Emitter::once<InitEvent>(std::function<void(InitEvent& event, Resource& handle)> callback) {
  if (inited_) {
    callback(init_event_, dynamic_cast<Resource&>(*this));
    return ;
  }
  auto q = getHandler<InitEvent>();
  q->once(std::move(callback));
}

} // namespace unio
} // namespace jcu
