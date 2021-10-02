/**
 * @file	buffer_unittest.cc
 * @author	Joseph Lee <joseph@jc-lab.net>
 * @date	2021-09-29
 * @copyright Copyright (C) 2021 jc-lab. All rights reserved.
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */

#include <string>
#include <atomic>

#include <gtest/gtest.h>

#include <jcu-unio/buffer.h>

namespace {

using namespace jcu::unio;

class BufferTest : public ::testing::Test {
};

TEST_F(BufferTest, PositionTest) {
  auto buffer = createFixedSizeBuffer(1024);
  char* begin = (char*) buffer->data();
  const void* p;

  EXPECT_EQ(begin, buffer->base());
  p =  buffer->base();
  EXPECT_EQ(begin, p);

  buffer->clear();
  EXPECT_EQ(buffer->position(), 0);

  buffer->position(128);
  EXPECT_EQ(buffer->position(), 128);
  p = buffer->data();
  EXPECT_EQ(p, begin + 128);

  EXPECT_EQ(buffer->remaining(), 1024 - 128);
  buffer->flip();

  EXPECT_EQ(buffer->position(), 0);
  EXPECT_EQ(buffer->remaining(), 128);

  buffer->limit(512);
  buffer->position(buffer->position() + 128);
  EXPECT_EQ(buffer->remaining(), 512 - 128);
}

}
