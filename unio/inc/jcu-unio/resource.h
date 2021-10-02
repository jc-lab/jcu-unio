/**
 * @file	resource.h
 * @author	Joseph Lee <joseph@jc-lab.net>
 * @date	2021-09-11
 * @copyright Copyright (C) 2021 jc-lab. All rights reserved.
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */


#ifndef JCU_UNIO_RESOURCE_H_
#define JCU_UNIO_RESOURCE_H_

#include <memory>

namespace jcu {
namespace unio {

class Loop;
class Logger;

struct BasicParams {
  std::shared_ptr<Loop> loop;
  std::shared_ptr<Logger> logger;
  //TODO: Memory Pool
};

class Resource {
 protected:
  BasicParams basic_params_;

 protected:
  virtual std::shared_ptr<Resource> sharedAsResource() = 0;

 public:
  virtual ~Resource() = default;
  virtual void close() = 0;
};

} // namespace unio
} // namespace jcu

#endif //JCU_UNIO_RESOURCE_H_
