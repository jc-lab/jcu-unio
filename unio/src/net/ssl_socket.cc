/**
 * @file	ssl_socket.cc
 * @author	Joseph Lee <joseph@jc-lab.net>
 * @date	2021-09-14
 * @copyright Copyright (C) 2021 jc-lab. All rights reserved.
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */

#include <uv/errno.h>

#include <jcu-unio/log.h>
#include <jcu-unio/net/ssl_socket.h>
#include <jcu-unio/net/ssl_context.h>

namespace jcu {
namespace unio {

class Logger;

class SSLSocketImpl : public SSLSocket {
 public:
  std::weak_ptr<SSLSocketImpl> self_;
  BasicParams basic_params_;

  std::shared_ptr<jcu::unio::StreamSocket> parent_;
  std::shared_ptr<SSLContext> ssl_context_;

  std::unique_ptr<SSLEngine> ssl_engine_;
  std::shared_ptr<Buffer> socket_outbound_buffer_;

  CompletionOnceCallback<jcu::unio::SocketConnectEvent> connect_event_;

  std::shared_ptr<Buffer> socket_inbound_buffer_;

  bool handshaked_;
  bool closing_;

  SSLSocketImpl(const BasicParams& basic_params, std::shared_ptr<SSLContext> ssl_context) :
      basic_params_(basic_params),
      ssl_context_(ssl_context),
      handshaked_(false),
      closing_(false),
      connect_event_(nullptr)
  {
    basic_params_.logger->logf(Logger::kLogTrace, "SSLSocketImpl construct");
  }

  ~SSLSocketImpl() {
    basic_params_.logger->logf(Logger::kLogTrace, "SSLSocketImpl destruct");
  }

  std::shared_ptr<SSLSocket> shared() const override {
    return self_.lock();
  }

  void _init() override {
    InitEvent event;
    emitInit(std::move(event));
  }

  void setParent(std::shared_ptr<jcu::unio::StreamSocket> parent) override {
    std::shared_ptr<SSLSocketImpl> self(self_.lock());
    parent_ = parent;
    parent_->once<InitEvent>([self](InitEvent& event, Resource& resource) -> void {
      self->init();
    });
    parent_->once<CloseEvent>([self](CloseEvent& event, Resource& handle) -> void {
      self->basic_params_.logger->logf(Logger::kLogTrace, "SSLSocket: parent's CloseEvent");
      self->closeImpl(true);
      self->emit<CloseEvent>(event);
      self->offAll();
    });
    parent_->on<SocketReadEvent>([self](SocketReadEvent& event, Resource& handle) -> void {
      if (event.hasError()) {
        self->emit<SocketReadEvent>(event);
        self->close();
        return ;
      }
      SSLEngine::DataResult result;
      Buffer* inbound_buffer;
      do {
        inbound_buffer = self->socket_inbound_buffer_.get();
        if (inbound_buffer) inbound_buffer->clear();
        result = self->ssl_engine_->unwrap(event.buffer(), inbound_buffer);
        if (result & SSLEngine::kDataRead) {
          if (inbound_buffer && inbound_buffer->remaining() > 0) {
            SocketReadEvent event {inbound_buffer};
            self->emit<SocketReadEvent>(event);
          }
        }
      } while(inbound_buffer && (result & SSLEngine::kDataReadMore));
      self->tlsProcess();
    });
  }

  void setSocketOutboundBuffer(std::shared_ptr<Buffer> buffer) override {
    socket_outbound_buffer_ = buffer;
  }

  void read(std::shared_ptr<Buffer> buffer) override {
    socket_inbound_buffer_ = buffer;
  }

  void cancelRead() override {
    socket_inbound_buffer_ = nullptr;
  }

  void write(std::shared_ptr<Buffer> buffer, CompletionOnceCallback<SocketWriteEvent> callback) override {
    std::shared_ptr<SSLSocketImpl> self(self_.lock());
    socket_outbound_buffer_->clear();
    int rc = ssl_engine_->wrap(buffer.get(), socket_outbound_buffer_.get());
    //TODO: partial write
    parent_->write(socket_outbound_buffer_, [self, callback = std::move(callback)](SocketWriteEvent& event, Resource& handle) mutable -> void {
      callback(event, *self);
    });
  }

  void emitConnectEvent(CompletionOnceCallback<SocketConnectEvent>& callback, SocketConnectEvent& event) {
    if (callback) {
      callback(event, *this);
    } else {
      if (event.hasError()) {
        emit<ErrorEvent>(event.error());
      } else {
        emit<SocketConnectEvent>(event);
      }
    }
  }

  void connect(
      std::shared_ptr<ConnectParam> connect_param,
      CompletionOnceCallback<jcu::unio::SocketConnectEvent> callback
  ) override {
    std::shared_ptr<SSLSocketImpl> self(self_.lock());
    parent_->connect(connect_param, [self, callback = std::move(callback), connect_param](jcu::unio::SocketConnectEvent& event, jcu::unio::Resource& handle) mutable -> void {
      if (event.hasError()) {
        self->emitConnectEvent(callback, event);
        return ;
      }

      self->connect_event_ = std::move(callback);

      self->ssl_engine_ = std::move(self->ssl_context_->createEngine(self->basic_params_, kSslClient));
      const char *hostname = connect_param->getHostname();
      if (hostname) self->ssl_engine_->setHostname(hostname);
      self->ssl_engine_->beginHandshake();

      auto buffer = createFixedSizeBuffer(8192);
      self->parent_->read(buffer);
      self->tlsProcess();
    });
  }

  int bind(std::shared_ptr<BindParam> bind_param) override {
    return UV__EINVAL;
  }

  int listen(int backlog) override {
    return UV__EINVAL;
  }

  void tlsProcess() {
    std::shared_ptr<SSLSocketImpl> self(self_.lock());
    int rc;
    SSLEngine::HandshakeStatus status = ssl_engine_->getHandshakeStatus();
    switch (status) {
      case SSLEngine::kHandshakeNeedWrap:
        if (!socket_outbound_buffer_) {
          socket_outbound_buffer_ = createFixedSizeBuffer(8192);
        }
        socket_outbound_buffer_->clear();
        rc = ssl_engine_->wrap(nullptr, socket_outbound_buffer_.get());
        if (socket_outbound_buffer_->remaining() > 0) {
          parent_->write(socket_outbound_buffer_, [self](SocketWriteEvent& event, Resource& handle) -> void {
            self->tlsProcess();
          });
        }
        break;
      case SSLEngine::kHandshakeNeedUnwrap:
        break;
      case SSLEngine::kHandshakeFinished:
        handshaked_ = true;
        if (connect_event_) {
          SocketConnectEvent event;
          emitConnectEvent(connect_event_, event);
          connect_event_ = nullptr;
        }
        break;
      case SSLEngine::kHandshakeClosing:
        close();
        break;
      case SSLEngine::kHandshakeFailed:
        if (connect_event_) {
          SocketConnectEvent event { ssl_engine_->getHandshakeError() };
          emitConnectEvent(connect_event_, event);
          connect_event_ = nullptr;
        }
    }
  }

  void emitDisconnectEvent(CompletionOnceCallback<SocketDisconnectEvent>& callback, SocketDisconnectEvent& event) {
    if (callback) {
      callback(event, *this);
    } else {
      if (event.hasError()) {
        emit<ErrorEvent>(event.error());
      } else {
        emit<SocketDisconnectEvent>(event);
      }
    }
  }

  void disconnect(CompletionOnceCallback<SocketDisconnectEvent> callback) override {
    cancelRead();
    if (ssl_engine_->shutdown()) {
      tlsProcess();
      SocketDisconnectEvent event;
      emitDisconnectEvent(callback, event);
    } else {
      SocketDisconnectEvent event { ssl_engine_->getHandshakeError() };
      emitDisconnectEvent(callback, event);
    }
  }

  int accept(std::shared_ptr<StreamSocket> client) override {
    return UV__EINVAL;
  }

  bool isConnected() const override {
    return parent_->isConnected();
  }

  bool isHandshaked() const override {
    return handshaked_;
  }

  void closeImpl(bool from_parent) {
    if (closing_) return ;
    closing_ = true;

    cancelRead();

    if (!from_parent) {
      parent_->close();
    }
  }

  void close() override {
    closeImpl(false);
  }
};

std::shared_ptr<SSLSocket> SSLSocket::create(
    const BasicParams& basic_params,
    std::shared_ptr<SSLContext> ssl_context
) {
  std::shared_ptr<SSLSocketImpl> instance(new SSLSocketImpl(basic_params, std::move(ssl_context)));
  instance->self_ = instance;
  return std::move(instance);
}

} // namespace unio
} // namespace jcu
