/**
 * @file	unit_test_utils.h
 * @author	Joseph Lee <joseph@jc-lab.net>
 * @date	2021-09-28
 * @copyright Copyright (C) 2021 jc-lab. All rights reserved.
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */


#ifndef JCU_UNIO_UNIO_SRC_UNIT_TEST_UTILS_H_
#define JCU_UNIO_UNIO_SRC_UNIT_TEST_UTILS_H_

#include <thread>
#include <chrono>
#include <atomic>
#include <future>

#include <gtest/gtest.h>

#include <jcu-unio/resource.h>

template <typename T>
struct destructive_copy_constructible
{
  mutable T value;

  destructive_copy_constructible() {}

  destructive_copy_constructible(T&& v): value(std::move(v)) {}

  destructive_copy_constructible(const destructive_copy_constructible<T>& rhs)
      : value(std::move(rhs.value))
  {}

  destructive_copy_constructible(destructive_copy_constructible<T>&& rhs) = default;

  destructive_copy_constructible&
  operator=(const destructive_copy_constructible<T>& rhs) = delete;

  destructive_copy_constructible&
  operator=(destructive_copy_constructible<T>&& rhs) = delete;
};

template <typename T>
using dcc_t =
destructive_copy_constructible<typename std::remove_reference<T>::type>;

template <typename T>
inline dcc_t<T> move_to_dcc(T&& r)
{
  return dcc_t<T>(std::move(r));
}

namespace jcu {
namespace unio {

class Loop;

class LoopSupportTest : public ::testing::Test {
 public:
  BasicParams basic_params_;

 protected:
  std::thread thread_;
  std::atomic_bool stopped_;

  void SetUp() override;
  void TearDown() override;
};

} // namespace unio
} // namespace jcu

#endif //JCU_UNIO_UNIO_SRC_UNIT_TEST_UTILS_H_
