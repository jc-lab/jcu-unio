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

class SocketConnectEvent : public BaseEvent {
 protected:
  UvErrorEvent error_;

 public:
  SocketConnectEvent();
  SocketConnectEvent(UvErrorEvent error);
  bool hasError() const override;
  ErrorEvent &error() override;
  const ErrorEvent &error() const override;
};

class SocketDisconnectEvent : public BaseEvent {
 protected:
  UvErrorEvent error_;

 public:
  SocketDisconnectEvent();
  SocketDisconnectEvent(UvErrorEvent error);
  bool hasError() const override;
  ErrorEvent &error() override;
  const ErrorEvent &error() const override;
};

template <class T>
class StreamSocket : public Socket<T> {
 public:
  virtual void connect(
      std::shared_ptr<ConnectParam> connect_param,
      CompletionOnceCallback<void(SocketConnectEvent& event, T& handle)> callback
  ) = 0;

  virtual void disconnect(
      CompletionOnceCallback<void(SocketDisconnectEvent& event, T& handle)> callback
  ) = 0;

  virtual bool isConnected() const = 0;
};

} // namespace unio
} // namespace jcu

#endif //JCU_UNIO_NET_STREAM_SOCKET_H_