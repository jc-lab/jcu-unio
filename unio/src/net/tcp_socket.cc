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
  typedef UvCallbackRef<uv_connect_t, SocketConnectEvent, TCPSocketImpl> ConnectCallbackRef;

  class HandleRef : public UvRef<uv_tcp_t, TCPSocketImpl> {
   public:
    HandleRef() : UvRef(nullptr) {}
    void close() override {
      data_.reset();
    }
    void setData(std::shared_ptr<TCPSocketImpl> data) {
      data_ = data;
    }
  };
  class WriteRef : public UvCallbackRef<uv_write_t, SocketWriteEvent, TCPSocketImpl> {
   public:
    uv_buf_t buf;
    WriteRef(std::shared_ptr<TCPSocketImpl> data) :
        UvCallbackRef(data)
    {
      std::memset(&buf, 0, sizeof(buf));
    }
//    void close() override {
//      data_.reset();
//    }
//    void setData(std::shared_ptr<TCPSocketImpl> data) {
//      data_ = data;
//    }
//    void publishOnce(SocketWriteEvent& event) {
//      publish(event);
//      fn_ = nullptr;
//    }
    static WriteRef* create(std::shared_ptr<TCPSocketImpl> data) {
      return new WriteRef(data);
    }
    static WriteRef* from(void* handle) {
      UvRefBase* base = (UvRefBase*) uv_handle_get_data((uv_handle_t*)handle);
      return dynamic_cast<WriteRef*>(base);
    }
  };

  std::shared_ptr<Loop> loop_;
  std::shared_ptr<Logger> log_;

  std::weak_ptr<TCPSocketImpl> self_;

  HandleRef handle_;
  std::shared_ptr<Buffer> read_buffer_;
  CompletionManyCallback<SocketReadEvent> read_callback_;

  bool connected_;

 public:
  TCPSocketImpl(std::shared_ptr<Loop> loop, std::shared_ptr<Logger> log) :
      loop_(loop),
      log_(log),
      connected_(false)
  {
    log_->logf(jcu::unio::Logger::kLogTrace, "TCPSocketImpl: construct");
  }

  ~TCPSocketImpl() {
    log_->logf(jcu::unio::Logger::kLogTrace, "TCPSocketImpl: destruct");
  }

  std::shared_ptr<TCPSocket> shared() const override {
    return self_.lock();
  }

  static void closeCallback(uv_handle_t* handle) {
    auto* ref = HandleRef::from(handle);
    std::shared_ptr<TCPSocketImpl> self = ref->data();
    ref->close();
    self->log_->logf(jcu::unio::Logger::kLogTrace, "TCPSocketImpl: closeCallback");
    CloseEvent event {};
    self->emit(event);
  }

  void close() override {
    connected_ = false;
    cancelRead();
    if (!uv_is_closing(handle_.handle<uv_handle_t>())) {
      uv_close(handle_.handle<uv_handle_t>(), closeCallback);
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
    buffer->clear();
    size_t buffer_remaining = buffer->remaining();
    if (buffer_remaining < suggested_size) {
      size_t suggested_new_size = buffer->capacity() + (suggested_size - buffer_remaining);
      size_t expandable_size = buffer->getExpandableSize();
      size_t new_size = (expandable_size >= suggested_new_size) ? suggested_new_size : expandable_size;
      buffer->expand(new_size);
    }
    buf->base = (char*) buffer->data();
    buf->len = buffer->remaining();
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
    if (self->read_callback_) {
      if (nread < 0) {
        SocketReadEvent event { UvErrorEvent { nread, 0 }, nullptr };
        self->read_callback_(event, *self);
        return;
      }
      buffer->limit(buffer->position() + nread);
      {
        SocketReadEvent event { buffer.get() };
        self->read_callback_(event, *self);
      }
      buffer->clear();
    }
  }

  void read(std::shared_ptr<Buffer> buffer, CompletionManyCallback<SocketReadEvent> callback) override {
    std::shared_ptr<TCPSocketImpl> self(self_.lock());
    read_buffer_ = buffer;
    read_callback_ = callback;
    loop_->sendQueuedTask([self]() -> void {
      uv_read_start(self->handle_.handle<uv_stream_t>(), allocCallback, readCallback);
    });
  }

  void cancelRead() override {
    uv_read_stop(handle_.handle<uv_stream_t>());
    read_callback_ = nullptr;
    read_buffer_.reset();
  }

  static void writeCallback(uv_write_t* req, int status) {
    auto ref = WriteRef::from(req);
    SocketWriteEvent event { UvErrorEvent { status, 0 } };
    ref->publishAndClose(event);
  }

  void write(std::shared_ptr<Buffer> buffer, CompletionOnceCallback<SocketWriteEvent> callback) override {
    auto ref = WriteRef::create(self_.lock());
    ref->buf.base = (char*)buffer->data();
    ref->buf.len = buffer->remaining();
    ref->reset(
        std::move(callback),
        &uv_write,
        handle_.handle<uv_stream_t>(),
        &ref->buf,
        1,
        writeCallback
    );
  }

  static void connectCallback(uv_connect_t* handle, int status) {
    auto* ref = ConnectCallbackRef::from(handle);
    auto self = ref->data();
    if (status == 0) {
      self->connected_ = true;
      SocketConnectEvent event {};
      ref->publishAndClose(event);
    } else {
      SocketConnectEvent event { std::make_shared<UvErrorEvent>(status, 0) };
      ref->publishAndClose(event);
    }
  }

  void connect(std::shared_ptr<ConnectParam> connect_param, CompletionOnceCallback<SocketConnectEvent> callback) override {
    std::shared_ptr<TCPSocketImpl> self(self_.lock());
    loop_->sendQueuedTask([self, connect_param, callback = std::move(callback)]() mutable -> void {
      int rc;
      self->handle_.setData(self);
      rc = uv_tcp_init(self->loop_->get(), self->handle_.handle());
      if (rc) {
        SocketConnectEvent event { UvErrorEvent { rc, 0 } };
        callback(event, *self);
        return ;
      }
      self->handle_.attach();

      ConnectCallbackRef::create(
          self,
          std::move(callback),
          &uv_tcp_connect, self->handle_.handle(), connect_param->getSockAddr(),
          connectCallback
      );
    });
  }

  void disconnect(CompletionOnceCallback<SocketDisconnectEvent> callback) override {
    typedef UvCallbackRef<uv_shutdown_t, SocketDisconnectEvent, TCPSocketImpl> CallbackRefImpl;
    std::shared_ptr<TCPSocketImpl> self(self_.lock());
    loop_->sendQueuedTask([self, callback = std::move(callback)]() mutable -> void {
      CallbackRefImpl::create(
          self,
          std::move(callback),
          &uv_shutdown, self->handle_.handle<uv_stream_t>(), [](uv_shutdown_t* handle, int status) -> void {
            auto* ref = CallbackRefImpl::from(handle);
            auto self = ref->data();
            self->connected_ = false;
            ref->publishAndClose(UvErrorEvent {status, 0 });
          }
      );
    });
  }

  bool isConnected() const override {
    return connected_;
  }

  static std::shared_ptr<TCPSocketImpl> create(std::shared_ptr<Loop> loop, std::shared_ptr<Logger> log) {
    auto instance = std::make_shared<TCPSocketImpl>(loop, log);
    instance->self_ = instance;
    return std::move(instance);
  }
};

std::shared_ptr<TCPSocket> TCPSocket::create(std::shared_ptr<Loop> loop, std::shared_ptr<Logger> log) {
  return std::move(TCPSocketImpl::create(loop, log));
}

} // namespace unio
} // namespace jcu
