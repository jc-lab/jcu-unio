/**
 * @file	resource.h
 * @author	Joseph Lee <joseph@jc-lab.net>
 * @date	2021-09-11
 * @copyright Copyright (C) 2021 jc-lab. All rights reserved.
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */


#ifndef OPENVPN_CLIENTPP_JCU_UNIO_UNIO_INC_JCU_UNIO_RESOURCE_H_
#define OPENVPN_CLIENTPP_JCU_UNIO_UNIO_INC_JCU_UNIO_RESOURCE_H_

namespace jcu {
namespace unio {

class Resource {
 public:
  virtual ~Resource() = default;
};

} // namespace unio
} // namespace jcu

#endif //OPENVPN_CLIENTPP_JCU_UNIO_UNIO_INC_JCU_UNIO_RESOURCE_H_
