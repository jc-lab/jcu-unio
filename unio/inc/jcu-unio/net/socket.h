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

#include <cstring>

#include "../buffer.h"
#include "../event.h"
#include "../handle.h"

namespace jcu {
namespace unio {

class SocketReadEvent : public AbstractEvent {
 protected:
  Buffer* buffer_;

 public:
  SocketReadEvent(std::shared_ptr<ErrorEvent> error, Buffer* buffer = nullptr);
  SocketReadEvent(Buffer* buffer);
  Buffer* buffer() {
    return buffer_;
  }
  const Buffer* buffer() const {
    return buffer_;
  }
};

class SocketWriteEvent : public AbstractEvent {
 public:
  SocketWriteEvent();
  SocketWriteEvent(std::shared_ptr<ErrorEvent> error);
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

class BindParam {
 private:
  BindParam(const BindParam&) = delete;

 public:
  BindParam() {}
  virtual ~BindParam() = default;
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

template<typename T>
class SockAddrBindParam : public BindParam {
 protected:
  T sockaddr_;

 public:
  SockAddrBindParam() {
    std::memset(&sockaddr_, 0, sizeof(sockaddr_));
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
  /**
   * Start reading
   * When there is data to read, SocketReadEvent is emitted.
   *
   * @param buffer read buffer
   */
  virtual void read(
      std::shared_ptr<Buffer> buffer
  ) = 0;

  /**
   * Cancel Read
   */
  virtual void cancelRead() = 0;

  /**
   * When the write is complete, the callback is called.
   * If the callback is nullptr,
   * a SocketWriteEvent is emitted if the write has completed successfully,
   * or an ErrorEvent if an error has occurred..
   *
   * @param buffer data to write.
   *               The buffer must not be modified until the write is complete.
   * @param callback
   */
  virtual void write(
      std::shared_ptr<Buffer> buffer,
      CompletionOnceCallback<SocketWriteEvent> callback = nullptr
  ) = 0;

  /**
   * bind
   *
   * @param bind_param
   * @return uv errno
   */
  virtual int bind(
      std::shared_ptr<BindParam> bind_param
  ) = 0;
};

} // namespace unio
} // namespace jcu

#endif //JCU_UNIO_NET_SOCKET_H_
