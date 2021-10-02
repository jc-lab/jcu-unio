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
 public:
  typedef UvCallbackRef<uv_connect_t, SocketConnectEvent, TCPSocketImpl> ConnectCallbackRef;
  typedef UvCallbackRef<uv_shutdown_t, SocketDisconnectEvent, TCPSocketImpl> ShutdownCallbackRef;

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

  std::weak_ptr<TCPSocketImpl> self_;

  HandleRef handle_;
  std::shared_ptr<Buffer> read_buffer_;

  bool connected_;

  TCPSocketImpl(const BasicParams& basic_params) :
      connected_(false)
  {
    basic_params_ = basic_params;
    basic_params_.logger->logf(jcu::unio::Logger::kLogTrace, "TCPSocketImpl: construct");
  }

  ~TCPSocketImpl() {
    basic_params_.logger->logf(jcu::unio::Logger::kLogTrace, "TCPSocketImpl: destruct");
  }

  std::shared_ptr<Resource> sharedAsResource() override {
    return self_.lock();
  }

  std::shared_ptr<TCPSocket> shared() const override {
    return self_.lock();
  }

  void _init() override {
    int rc;
    rc = uv_tcp_init(basic_params_.loop->get(), handle_.handle());
    if (rc == 0) {
      handle_.setData(self_.lock());
      handle_.attach();
    }
    InitEvent event { UvErrorEvent::createIfNeeded(rc) };
    emitInit(std::move(event));
  }

  static void closeCallback(uv_handle_t* handle) {
    auto* ref = HandleRef::from(handle);
    std::shared_ptr<TCPSocketImpl> self = ref->data();
    self->basic_params_.logger->logf(jcu::unio::Logger::kLogTrace, "TCPSocketImpl: closeCallback");
    CloseEvent event {};
    self->emit<CloseEvent>(event);
    self->offAll();
    ref->close();
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
    if (nread == UV_EOF) {
      SocketEndEvent event;
      self->emit(event);
      return ;
    } else if (nread < 0) {
      auto error_event = UvErrorEvent::createIfNeeded(nread, 0);
      self->emit<ErrorEvent>(*error_event);
      return;
    }
    buffer->limit(buffer->position() + nread);
    {
      SocketReadEvent event { buffer.get() };
      self->emit<SocketReadEvent>(event);
    }
    buffer->clear();
  }

  void read(std::shared_ptr<Buffer> buffer) override {
    std::shared_ptr<TCPSocketImpl> self(self_.lock());
    read_buffer_ = buffer;
    basic_params_.loop->sendQueuedTask([self]() -> void {
      uv_read_start(self->handle_.handle<uv_stream_t>(), allocCallback, readCallback);
    });
  }

  void cancelRead() override {
    uv_read_stop(handle_.handle<uv_stream_t>());
    read_buffer_.reset();
  }

  static void writeCallback(uv_write_t* req, int status) {
    auto ref = WriteRef::from(req);
    std::shared_ptr<TCPSocketImpl> self(ref->data());
    SocketWriteEvent event { UvErrorEvent::createIfNeeded(status, 0) };
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
    }

    SocketConnectEvent event { UvErrorEvent::createIfNeeded(status, 0) };
    ref->publishAndClose(event);
  }

  void connect(std::shared_ptr<ConnectParam> connect_param, CompletionOnceCallback<SocketConnectEvent> callback) override {
    std::shared_ptr<TCPSocketImpl> self(self_.lock());
    ConnectCallbackRef::create(
        self,
        std::move(callback),
        &uv_tcp_connect, self->handle_.handle(), connect_param->getSockAddr(),
        connectCallback
    );
  }

  static void shutdownCallback(uv_shutdown_t* handle, int status) {
    auto* ref = ShutdownCallbackRef::from(handle);
    auto self = ref->data();
    self->connected_ = false;
    SocketDisconnectEvent event { UvErrorEvent::createIfNeeded(status, 0) };
    ref->publishAndClose(event);
  }

  void disconnect(CompletionOnceCallback<SocketDisconnectEvent> callback) override {
    std::shared_ptr<TCPSocketImpl> self(self_.lock());
    ShutdownCallbackRef::create(
        self,
        std::move(callback),
        &uv_shutdown, self->handle_.handle<uv_stream_t>(), shutdownCallback
    );
  }

  int bind(std::shared_ptr<BindParam> bind_param) override {
    return uv_tcp_bind(handle_.handle(), bind_param->getSockAddr(), 0);
  }

  static void listenCallback(uv_stream_t* server, int status) {
    auto ref = HandleRef::from(server);
    std::shared_ptr<TCPSocketImpl> self(ref->data());
    SocketListenEvent event { UvErrorEvent::createIfNeeded(status) };
    self->emit<SocketListenEvent>(event);
  }

  int listen(int backlog) override {
    return uv_listen(handle_.handle<uv_stream_t>(), backlog, listenCallback);
  }

  int accept(std::shared_ptr<StreamSocket> client) override {
    auto impl = std::dynamic_pointer_cast<TCPSocketImpl>(client);
    return uv_accept(handle_.handle<uv_stream_t>(), impl->handle_.handle<uv_stream_t>());
  }

  bool isConnected() const override {
    return connected_;
  }
};

std::shared_ptr<TCPSocket> TCPSocket::create(const BasicParams& basic_params) {
  auto instance = std::make_shared<TCPSocketImpl>(basic_params);
  instance->self_ = instance;
  basic_params.loop->sendQueuedTask([instance]() -> void {
    instance->init();
  });
  return std::move(instance);
}

} // namespace unio
} // namespace jcu
