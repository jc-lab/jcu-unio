/**
 * @file	shared_object.h
 * @author	Joseph Lee <joseph@jc-lab.net>
 * @date	2021-09-11
 * @copyright Copyright (C) 2021 jc-lab. All rights reserved.
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */


#ifndef JCU_UNIO_SHARED_OBJECT_H_
#define JCU_UNIO_SHARED_OBJECT_H_

#include <memory>

namespace jcu {
namespace unio {

/**
 * @tparam T The class of the final implementation
 */
template<class T>
class SharedObject {
 public:
  virtual std::shared_ptr<T> shared() const = 0;
};

} // namespace unio
} // namespace jcu

#endif //JCU_UNIO_SHARED_OBJECT_H_
