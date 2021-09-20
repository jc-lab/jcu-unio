/**
 * @file	log.h
 * @author	Joseph Lee <joseph@jc-lab.net>
 * @date	2021-09-11
 * @copyright Copyright (C) 2021 jc-lab. All rights reserved.
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */

#ifndef JCU_UNIO_SRC_LOG_H_
#define JCU_UNIO_SRC_LOG_H_

#include <jcu-unio/log.h>

namespace jcu {
namespace unio {
namespace intl {

std::shared_ptr<Logger> createNullLogger();

} // namespace intl
} // namespace unio
} // namespace jcu

#endif //JCU_UNIO_SRC_LOG_H_
