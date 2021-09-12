/**
 * @file	tcp_socket.h
 * @author	Joseph Lee <joseph@jc-lab.net>
 * @date	2021-09-12
 * @copyright Copyright (C) 2021 jc-lab. All rights reserved.
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */


#ifndef OPENVPN_CLIENTPP_JCU_UNIO_UNIO_INC_JCU_UNIO_NET_TCP_SOCKET_H_
#define OPENVPN_CLIENTPP_JCU_UNIO_UNIO_INC_JCU_UNIO_NET_TCP_SOCKET_H_

#include "stream_socket.h"

namespace jcu {
namespace unio {

class Logger;

class TCPSocket : public StreamSocket<TCPSocket> {
 public:
  static std::shared_ptr<TCPSocket> create(std::shared_ptr<Loop> loop, std::shared_ptr<Logger> log);
};

} // namespace unio
} // namespace jcu

#endif //OPENVPN_CLIENTPP_JCU_UNIO_UNIO_INC_JCU_UNIO_NET_TCP_SOCKET_H_
