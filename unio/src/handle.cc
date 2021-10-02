/**
 * @file	handle.cc
 * @author	Joseph Lee <joseph@jc-lab.net>
 * @date	2021-09-11
 * @copyright Copyright (C) 2021 jc-lab. All rights reserved.
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */

#include <jcu-unio/loop.h>
#include <jcu-unio/handle.h>

namespace jcu {
namespace unio {

void Handle::invokeInitEventCallback(std::function<void(InitEvent &, Resource &)> &&callback, InitEvent& event) {
  std::shared_ptr<Resource> self(sharedAsResource());
  basic_params_.loop->sendQueuedTask([self, callback = std::move(callback), &event]() mutable -> void {
    callback(event, *self);
  });
}

} // namespace unio
} // namespace jcu
