/**
 * @file	buffer.cc
 * @author	Joseph Lee <joseph@jc-lab.net>
 * @date	2021-09-12
 * @copyright Copyright (C) 2021 jc-lab. All rights reserved.
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */

#include <vector>

#include <jcu-unio/buffer.h>

namespace jcu {
namespace unio {

class VectorBuffer : public Buffer {
 protected:
  std::vector<char> buf_;
  size_t expandable_size_;
  size_t position_;
  size_t limit_;

 public:
  VectorBuffer(size_t initial_size, size_t expandable_size) :
      expandable_size_(expandable_size),
      position_(0),
      limit_(0)
  {
    buf_.resize(initial_size);
  }

  void *base() override {
    return buf_.data();
  }

  const void *base() const override {
    return buf_.data();
  }

  void *data() override {
    return buf_.data() + position();
  }

  const void *data() const override {
    return buf_.data() + position();
  }

  size_t capacity() const override {
    return buf_.size();
  }

  size_t position() const override {
    return position_;
  }

  void position(size_t size) override {
    position_ = size;
  }

  void limit(size_t size) override {
    limit_ = size;
  }

  size_t remaining() const override {
    return limit_ - position_;
  }

  void flip() override {
    limit_ = position_;
    position_ = 0;
  }

  void clear() override {
    position_ = 0;
    limit_ = capacity();
  }

  size_t getExpandableSize() const override {
    return expandable_size_;
  }

  void expand(size_t size) override {
    size_t expandable_size = getExpandableSize();
    size_t new_size = (size <= expandable_size) ? size : expandable_size;
    if ((new_size <= buf_.size()) || (expandable_size <= capacity())) {
      return ;
    }
    buf_.resize(new_size);
  }
};

std::shared_ptr<Buffer> createFixedSizeBuffer(size_t size) {
  return std::make_shared<VectorBuffer>(size, size);
}

std::shared_ptr<Buffer> createExpandableBuffer(size_t initial_size, size_t expandable_size) {
  return std::make_shared<VectorBuffer>(initial_size, expandable_size);
}

} // namespace unio
} // namespace jcu
