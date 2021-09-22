/**
 * @file	loop.h
 * @author	Joseph Lee <joseph@jc-lab.net>
 * @date	2021-08-28
 * @copyright Copyright (C) 2021 jc-lab. All rights reserved.
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */

#ifndef JCU_UNIO_LOOP_H_
#define JCU_UNIO_LOOP_H_

#include <memory>
#include <functional>

#include <uv.h>

#include "shared_object.h"

namespace jcu {
namespace unio {

typedef std::function<void()> QueuedTask_t;

struct LoopContext;
class Loop : public SharedObject<Loop> {
 protected:
  std::unique_ptr<LoopContext> ctx_;

 public:
  Loop();
  virtual ~Loop() = default;
  virtual uv_loop_t *get() const = 0;

  /**
   * Initialization for queued tasks.
   * It is safe to call it from a loop thread.
   */
  void init();
  /**
   * Unnitialization for queued tasks.
   */
  void uninit();

  /**
   * The uv_xxx_init function is not thread-safe.
   * So use this method to initialize the handle in the loop thread.
   */
  void sendQueuedTask(QueuedTask_t&& task) const;
};

class SharedLoop : public Loop {
 public:
  static std::shared_ptr<SharedLoop> create();
};

class UnsafeLoop : public Loop {
 public:
  static std::shared_ptr<UnsafeLoop> create(uv_loop_t* loop);
  static std::shared_ptr<UnsafeLoop> fromDefault();
};

} // namespace unio
} // namespace jcu

#endif //JCU_UNIO_LOOP_H_
