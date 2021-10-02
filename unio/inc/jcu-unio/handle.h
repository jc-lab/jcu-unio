/**
 * @file	handle.h
 * @author	Joseph Lee <joseph@jc-lab.net>
 * @date	2021-09-11
 * @copyright Copyright (C) 2021 jc-lab. All rights reserved.
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */


#ifndef JCU_UNIO_HANDLE_H_
#define JCU_UNIO_HANDLE_H_

#include <mutex>

#include "resource.h"
#include "emitter.h"

namespace jcu {
namespace unio {

class Loop;

/**
 * common events
 * - CloseEvent
 * - InitEvent
 */
class Handle : public Resource, public Emitter {
 protected:
  std::mutex init_mtx_;

  std::mutex& getInitMutex() override {
    return init_mtx_;
  }

  void invokeInitEventCallback(std::function<void(InitEvent& event, Resource& handle)>&& callback, InitEvent& event) override;

  virtual void _init() = 0;
 public:
  /**
   * initialize handle
   *
   * It is called automatically, but you can call it manually if you need it
   * It must be called from the loop thread.
   */
  virtual void init() {
    std::unique_lock<std::mutex> lock(init_mtx_);
    if (!inited_) {
      _init();
    }
  }
};

} // namespace unio
} // namespace jcu

#endif //JCU_UNIO_HANDLE_H_
