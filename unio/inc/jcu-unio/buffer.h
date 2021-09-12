/**
 * @file	buffer.h
 * @author	Joseph Lee <joseph@jc-lab.net>
 * @date	2021-09-12
 * @copyright Copyright (C) 2021 jc-lab. All rights reserved.
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */

#ifndef JCU_UNIO_BUFFER_H_
#define JCU_UNIO_BUFFER_H_

#include <stddef.h>

#include <memory>

namespace jcu {
namespace unio {

/**
 * IO Buffer
 * example)
 * - size that socket can read = capacity - position
 * - bytes available after IO read completes: position
 */
class Buffer {
 public:
  virtual ~Buffer() = default;
  virtual void* data() = 0;
  virtual const void* data() const = 0;
  virtual size_t capacity() const = 0;
  virtual size_t position() const = 0;
  virtual void position(size_t size) = 0;
  virtual void limit(size_t size) = 0;
  virtual size_t remaining() const = 0;
  virtual void flip() = 0;
  virtual void clear() = 0;

  /**
   * return maximum expandable size
   *
   * @return expandable bytes
   *         If it is less than or equal to capacity,e
   *         xpansion is not possible.
   */
  virtual size_t getExpandableSize() const = 0;

  /**
   * Expand the buffer
   *
   * @param size
   */
  virtual void expand(size_t size) = 0;
};

std::shared_ptr<Buffer> createFixedSizeBuffer(size_t size);
std::shared_ptr<Buffer> createExpandableBuffer(size_t initial_size, size_t expandable_size);

} // namespace unio
} // namespace jcu

#endif //JCU_UNIO_IO_BUFFER_H_
