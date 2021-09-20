/**
 * @file	socket.h
 * @author	Joseph Lee <joseph@jc-lab.net>
 * @date	2021-09-12
 * @copyright Copyright (C) 2021 jc-lab. All rights reserved.
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */


#ifndef JCU_UNIO_NET_SOCKET_H_
#define JCU_UNIO_NET_SOCKET_H_

#include "../buffer.h"
#include "../event.h"
#include "../handle.h"

namespace jcu {
namespace unio {

class SocketReadEvent : public BaseEvent {
 protected:
  UvErrorEvent error_;
  Buffer* buffer_;

 public:
  SocketReadEvent(UvErrorEvent error, Buffer* buffer);
  SocketReadEvent(Buffer* buffer);
  bool hasError() const override;
  ErrorEvent &error() override;
  const ErrorEvent &error() const override;
  Buffer* buffer() {
    return buffer_;
  }
  const Buffer* buffer() const {
    return buffer_;
  }
};

class SocketWriteEvent : public BaseEvent {
 protected:
  UvErrorEvent error_;

 public:
  SocketWriteEvent();
  SocketWriteEvent(UvErrorEvent error);
  bool hasError() const override;
  ErrorEvent &error() override;
  const ErrorEvent &error() const override;
};

class ConnectParam {
 private:
  ConnectParam(const ConnectParam&) = delete;

 public:
  ConnectParam() {}
  virtual ~ConnectParam() = default;
  virtual const char* getHostname() const = 0;
  virtual const struct sockaddr* getSockAddr() const = 0;
};

template<typename T>
class SockAddrConnectParam : public ConnectParam {
 protected:
  T sockaddr_;
  std::string hostname_;

 public:
  SockAddrConnectParam() {
    std::memset(&sockaddr_, 0, sizeof(sockaddr_));
  }

  void setHostname(const std::string& hostname) {
    hostname_ = hostname;
  }

  const char* getHostname() const {
    if (hostname_.empty()) return nullptr;
    return hostname_.c_str();
  }

  const sockaddr *getSockAddr() const override {
    return (sockaddr*) &sockaddr_;
  }

  T *getSockAddr() {
    return &sockaddr_;
  }
};

class Socket : public Handle {
 public:
  virtual void read(
      std::shared_ptr<Buffer> buffer,
      CompletionManyCallback<SocketReadEvent> callback
  ) = 0;
  virtual void cancelRead() = 0;

  /**
   * MUST call after the previous write operation is done.
   * @param buffer
   * @param callback
   */
  virtual void write(
      std::shared_ptr<Buffer> buffer,
      CompletionOnceCallback<SocketWriteEvent> callback
  ) = 0;
};

} // namespace unio
} // namespace jcu

#endif //JCU_UNIO_NET_SOCKET_H_
