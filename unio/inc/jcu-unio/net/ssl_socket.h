/**
 * @file	ssl_socket.h
 * @author	Joseph Lee <joseph@jc-lab.net>
 * @date	2021-09-14
 * @copyright Copyright (C) 2021 jc-lab. All rights reserved.
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */


#ifndef JCU_UNIO_NET_SSL_SOCKET_H_
#define JCU_UNIO_NET_SSL_SOCKET_H_

#include "../shared_object.h"
#include "stream_socket.h"

namespace jcu {
namespace unio {

class Loop;
class Logger;
class SSLContext;

class SSLSocket : public StreamSocket, public SharedObject<SSLSocket> {
 public:
  static std::shared_ptr<SSLSocket> create(
      const BasicParams& basic_params,
      std::shared_ptr<SSLContext> ssl_context
  );

  virtual void setParent(std::shared_ptr<jcu::unio::StreamSocket> parent) = 0;
  virtual void setSocketOutboundBuffer(std::shared_ptr<Buffer> buffer) = 0;
};

} // namespace unio
} // namespace jcu

#endif //JCU_UNIO_NET_SSL_SOCKET_H_
