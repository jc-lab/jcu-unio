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

#include <uv.h>

#include <functional>
#include <cstring>

#include "resource.h"
#include "emitter.h"

namespace jcu {
namespace unio {

class Loop;

/**
 * Common Events: CloseEvent.
 *
 * @tparam T The class of the final implementation
 */
template<class T>
class Handle : public Resource, public Emitter<T> {
 public:
  virtual std::shared_ptr<T> shared() const = 0;
  virtual void close() = 0;
};

} // namespace unio
} // namespace jcu

#endif //JCU_UNIO_HANDLE_H_
