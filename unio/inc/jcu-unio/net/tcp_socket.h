/**
 * @file	tcp_socket.h
 * @author	Joseph Lee <joseph@jc-lab.net>
 * @date	2021-09-12
 * @copyright Copyright (C) 2021 jc-lab. All rights reserved.
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */


#ifndef JCU_UNIO_NET_TCP_SOCKET_H_
#define JCU_UNIO_NET_TCP_SOCKET_H_

#include "../shared_object.h"
#include "stream_socket.h"

namespace jcu {
namespace unio {

class Loop;
class Logger;

class TCPSocket : public StreamSocket, public SharedObject<TCPSocket> {
 public:
  static std::shared_ptr<TCPSocket> create(const BasicParams& basic_params);
};

} // namespace unio
} // namespace jcu

#endif //JCU_UNIO_NET_TCP_SOCKET_H_
