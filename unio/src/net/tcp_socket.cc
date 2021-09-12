/**
 * @file	tcp_socket.cc
 * @author	Joseph Lee <joseph@jc-lab.net>
 * @date	2021-09-12
 * @copyright Copyright (C) 2021 jc-lab. All rights reserved.
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */

#include <jcu-unio/loop.h>
#include <jcu-unio/log.h>
#include <jcu-unio/uv_helper.h>
#include <jcu-unio/net/tcp_socket.h>

namespace jcu {
namespace unio {

class TCPSocketImpl : public TCPSocket {
 private:
  typedef UvRef<uv_tcp_t, TCPSocketImpl> HandleRef;

  class WriteRef : public UvRef<uv_write_t, TCPSocketImpl, CompletionOnceCallback<void(SocketWriteEvent &, TCPSocket &)>> {
   public:
    uv_buf_t buf;
    WriteRef(std::shared_ptr<TCPSocketImpl> data) :
      UvRef(data)
    {
      std::memset(&buf, 0, sizeof(buf));
    }
  };

  std::shared_ptr<Loop> loop_;
  std::shared_ptr<Logger> log_;

  std::weak_ptr<TCPSocketImpl> self_;

  HandleRef* handle_;
  std::shared_ptr<Buffer> read_buffer_;
  CompletionManyCallback<void(SocketReadEvent &, TCPSocket &)> read_callback_;

  WriteRef* write_ctx_;

  bool connected_;

 public:
  TCPSocketImpl(std::shared_ptr<Loop> loop, std::shared_ptr<Logger> log) :
      loop_(loop),
      log_(log),
      handle_(nullptr),
      connected_(false),
      write_ctx_(nullptr)
  {
    log_->logf(jcu::unio::Logger::kLogTrace, "TCPSocketImpl: construct");
  }

  ~TCPSocketImpl() {
    log_->logf(jcu::unio::Logger::kLogTrace, "TCPSocketImpl: destruct");
  }

  void init() {
    std::shared_ptr<TCPSocketImpl> self(self_.lock());
    loop_->sendQueuedTask([self]() -> void {
      self->handle_ = HandleRef::create(self);
      uv_tcp_init(self->loop_->get(), self->handle_->handle());
      self->handle_->attach();
      self->write_ctx_ = new WriteRef(self);
    });
  }

  std::shared_ptr<TCPSocket> shared() const override {
    return self_.lock();
  }

  void close() override {
    connected_ = false;
    if (!uv_is_closing(handle_->handle<uv_handle_t>())) {
      uv_close(handle_->handle<uv_handle_t>(), closeCallback);
    }
  }

  static void allocCallback(
      uv_handle_t* handle,
      size_t suggested_size,
      uv_buf_t* buf
  )
  {
    auto* ref = HandleRef::from(handle);
    auto self = ref->data();
    std::shared_ptr<Buffer> buffer = self->read_buffer_;
    if (buffer->capacity() < suggested_size) {
      size_t expandable_size = buffer->getExpandableSize();
      size_t new_size = (expandable_size >= suggested_size) ? suggested_size : expandable_size;
      buffer->expand(new_size);
    }
    buf->base = (char*) buffer->data();
    buf->len = buffer->capacity();
  }

  static void readCallback(
      uv_stream_t* stream,
      ssize_t nread,
      const uv_buf_t* buf
  )
  {
    auto* ref = HandleRef::from(stream);
    auto self = ref->data();
    auto buffer = self->read_buffer_;
    if (nread < 0) {
      self->read_callback_(SocketReadEvent { UvErrorEvent { nread, 0 }, nullptr }, *self);
      return ;
    }
    buffer->limit(buffer->position() + nread);
    self->read_callback_(SocketReadEvent { buffer.get() }, *self);
    buffer->clear();
  }

  void read(std::shared_ptr<Buffer> buffer, CompletionManyCallback<void(SocketReadEvent &, TCPSocket &)> callback) override {
    std::shared_ptr<TCPSocketImpl> self(self_.lock());
    read_buffer_ = buffer;
    read_callback_ = callback;
    loop_->sendQueuedTask([self]() -> void {
      uv_read_start(self->handle_->handle<uv_stream_t>(), allocCallback, readCallback);
    });
  }

  void cancelRead() override {
    std::shared_ptr<TCPSocketImpl> self(self_.lock());
    loop_->sendQueuedTask([self]() -> void {
      uv_read_stop(self->handle_->handle<uv_stream_t>());
      self->read_callback_ = nullptr;
      self->read_buffer_.reset();
    });
  }

  static void writeCallback(uv_write_t* req, int status) {
    auto *ref = WriteRef::from(req);
    ref->invoke(SocketWriteEvent { UvErrorEvent { status, 0 }}, *ref->data());
  }

  void write(std::shared_ptr<Buffer> buffer, CompletionOnceCallback<void(SocketWriteEvent &, TCPSocket &)> callback) override {
    write_ctx_->fn(std::move(callback));
    write_ctx_->buf.base = (char*)buffer->data();
    write_ctx_->buf.len = buffer->remaining();
    int rc = uv_write(
        write_ctx_->handle(),
        handle_->handle<uv_stream_t>(),
        &write_ctx_->buf,
        1,
        writeCallback
    );
    write_ctx_->attach();
    if (rc) {
      write_ctx_->invoke(SocketWriteEvent { UvErrorEvent { rc, 0 }}, *this);
    }
  }

  static void connectCallback(uv_connect_t* handle, int status) {
    auto* ref = UvRef<uv_connect_t, TCPSocketImpl, CompletionOnceCallback<void(SocketConnectEvent &, TCPSocket &)>>::from(handle);
    auto self = ref->data();
    if (status == 0) {
      self->connected_ = true;
    }
    ref->invoke(SocketConnectEvent { UvErrorEvent { status, 0 } }, *self);
    ref->close();
  }

  void connect(std::shared_ptr<ConnectParam> connect_param, CompletionOnceCallback<void(SocketConnectEvent &, TCPSocket &)> callback) override {
    std::shared_ptr<TCPSocketImpl> self(self_.lock());
    loop_->sendQueuedTask([self, connect_param, callback = std::move(callback)]() mutable -> void {
      auto* ref = UvRef<uv_connect_t, TCPSocketImpl, CompletionOnceCallback<void(SocketConnectEvent &, TCPSocket &)>>::create(self, std::move(callback));
      int rc = uv_tcp_connect(ref->handle(), self->handle_->handle(), connect_param->getSockAddr(), connectCallback);
      if (rc) {
        ref->invoke(SocketConnectEvent { UvErrorEvent { rc, 0 } }, *self);
        ref->close();
        return ;
      }
      ref->attach();
    });
  }

  static void closeCallback(uv_handle_t* handle) {
    auto* ref = UvRef<uv_tcp_t , TCPSocketImpl>::from(handle);
    auto self = ref->data();
    ref->close();
    if (self->write_ctx_) {
      self->write_ctx_->close();
      self->write_ctx_ = nullptr;
    }
    self->connected_ = false;
    self->read_buffer_.reset();
    self->read_callback_ = nullptr;
    self->log_->logf(jcu::unio::Logger::kLogTrace, "TCPSocketImpl: closeCallback");
    self->emit<CloseEvent>(CloseEvent{}, *self);
  }

  static void shutdownCallback(uv_shutdown_t* req, int status) {
    auto* ref = UvRef<uv_shutdown_t, TCPSocketImpl, CompletionOnceCallback<void(SocketDisconnectEvent &, TCPSocket &)>>::from(req);
    auto self = ref->data();
    int closing = uv_is_closing((uv_handle_t*) req->handle);
    self->log_->logf(jcu::unio::Logger::kLogTrace, "TCPSocketImpl: shutdownCallback: status=%d, closing=%d", status, closing);
    self->connected_ = false;
    ref->invoke(SocketDisconnectEvent { UvErrorEvent { status, 0 }}, *self);
    ref->close();
  }

  void disconnect(CompletionOnceCallback<void(SocketDisconnectEvent &, TCPSocket &)> callback) override {
    std::shared_ptr<TCPSocketImpl> self(self_.lock());
    auto* ref = UvRef<uv_shutdown_t, TCPSocketImpl, CompletionOnceCallback<void(SocketDisconnectEvent &, TCPSocket &)>>::create(self, std::move(callback));
    int rc = uv_shutdown(ref->handle(), handle_->handle<uv_stream_t>(), shutdownCallback);
    if (rc) {
      ref->invoke(SocketDisconnectEvent { UvErrorEvent { rc, 0 } }, *this);
      ref->close();
      return ;
    }
    ref->attach();
  }

  bool isConnected() const override {
    return connected_;
  }

  static std::shared_ptr<TCPSocketImpl> create(std::shared_ptr<Loop> loop, std::shared_ptr<Logger> log) {
    auto instance = std::make_shared<TCPSocketImpl>(loop, log);
    instance->self_ = instance;
    instance->init();
    return std::move(instance);
  }
};

std::shared_ptr<TCPSocket> TCPSocket::create(std::shared_ptr<Loop> loop, std::shared_ptr<Logger> log) {
  return std::move(TCPSocketImpl::create(loop, log));
}

} // namespace unio
} // namespace jcu
