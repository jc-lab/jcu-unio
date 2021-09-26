/**
 * @file	stream_socket.h
 * @author	Joseph Lee <joseph@jc-lab.net>
 * @date	2021-09-12
 * @copyright Copyright (C) 2021 jc-lab. All rights reserved.
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */


#ifndef JCU_UNIO_NET_STREAM_SOCKET_H_
#define JCU_UNIO_NET_STREAM_SOCKET_H_

#include "socket.h"

namespace jcu {
namespace unio {

class SocketConnectEvent : public AbstractEvent {
 public:
  SocketConnectEvent();
  SocketConnectEvent(std::shared_ptr<ErrorEvent> error);
};

class SocketDisconnectEvent : public AbstractEvent {
 public:
  SocketDisconnectEvent();
  SocketDisconnectEvent(std::shared_ptr<ErrorEvent> error);
};

class StreamSocket : public Socket {
 public:
 /**
  * When the connect is complete, the callback is called.
  * If the callback is nullptr,
  * a SocketConnectEvent is emitted if the connect has completed successfully,
  * or an ErrorEvent if an error has occurred.
  *
  * @param connect_param
  * @param callback
  */
  virtual void connect(
      std::shared_ptr<ConnectParam> connect_param,
      CompletionOnceCallback<SocketConnectEvent> callback
  ) = 0;

  /**
   * When the disconnect is complete, the callback is called.
   * If the callback is nullptr,
   * a SocketDisconnectEvent is emitted if the disconnect has completed successfully,
   * or an ErrorEvent if an error has occurred.
   *
   * @param callback
   */
  virtual void disconnect(
      CompletionOnceCallback<SocketDisconnectEvent> callback
  ) = 0;

  /**
   * When the disconnect is complete, the callback is called.
   * If the callback is nullptr,
   * a SocketListenEvent is emitted if the disconnect has completed successfully,
   * or an ErrorEvent if an error has occurred.
   *
   * @param callback
   */
  virtual int listen(
      int backlog
  ) = 0;

  virtual int accept(
      std::shared_ptr<StreamSocket> client
  ) = 0;

  virtual bool isConnected() const = 0;

  virtual bool isHandshaked() const {
    return isConnected();
  }
};

} // namespace unio
} // namespace jcu

#endif //JCU_UNIO_NET_STREAM_SOCKET_H_
