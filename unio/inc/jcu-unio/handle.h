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

#include "resource.h"
#include "emitter.h"

namespace jcu {
namespace unio {

/**
 * common events
 * - CloseEvent
 */
class Handle : public Resource, public Emitter {
};

} // namespace unio
} // namespace jcu

#endif //JCU_UNIO_HANDLE_H_
