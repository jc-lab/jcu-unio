/**
 * @file	ssl_socket.cc
 * @author	Joseph Lee <joseph@jc-lab.net>
 * @date	2021-09-14
 * @copyright Copyright (C) 2021 jc-lab. All rights reserved.
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */

#include <jcu-unio/log.h>
#include <jcu-unio/net/ssl_socket.h>
#include <jcu-unio/net/ssl_context.h>

namespace jcu {
namespace unio {

class Logger;

class SSLSocketImpl : public SSLSocket {
 public:
  std::weak_ptr<SSLSocketImpl> self_;
  std::shared_ptr<Loop> loop_;
  std::shared_ptr<Logger> log_;

  std::shared_ptr<jcu::unio::StreamSocket> parent_;
  std::shared_ptr<SSLContext> ssl_context_;

  std::unique_ptr<SSLEngine> ssl_engine_;
  std::shared_ptr<Buffer> socket_outbound_buffer_;

  CompletionOnceCallback<jcu::unio::SocketConnectEvent> connect_event_;

  std::shared_ptr<Buffer> socket_inbound_buffer_;
  CompletionManyCallback<SocketReadEvent> read_event_callback_;

  bool handshaked_;
  bool closing_;

  SSLSocketImpl(std::shared_ptr<Loop> loop, std::shared_ptr<Logger> log, std::shared_ptr<SSLContext> ssl_context) :
      loop_(loop),
      log_(log),
      ssl_context_(ssl_context),
      handshaked_(false),
      closing_(false),
      connect_event_(nullptr)
  {
    log_->logf(Logger::kLogTrace, "SSLSocketImpl construct");
  }

  ~SSLSocketImpl() {
    log_->logf(Logger::kLogTrace, "SSLSocketImpl destruct");
  }

  std::shared_ptr<SSLSocket> shared() const override {
    return self_.lock();
  }

  void setParent(std::shared_ptr<jcu::unio::StreamSocket> parent) override {
    std::shared_ptr<SSLSocketImpl> self(self_.lock());
    parent_ = parent;
    parent_->once<CloseEvent>([self](CloseEvent& event, Resource& handle) -> void {
      self->log_->logf(Logger::kLogTrace, "SSLSocket: parent's CloseEvent");
      self->closeImpl(true);
    });
  }

  void setSocketOutboundBuffer(std::shared_ptr<Buffer> buffer) override {
    socket_outbound_buffer_ = buffer;
  }

  void read(std::shared_ptr<Buffer> buffer, CompletionManyCallback<SocketReadEvent> callback) override {
    socket_inbound_buffer_ = buffer;
    read_event_callback_ = std::move(callback);
  }

  void cancelRead() override {
    socket_inbound_buffer_ = nullptr;
    read_event_callback_ = nullptr;
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

  void connect(
      std::shared_ptr<ConnectParam> connect_param,
      CompletionOnceCallback<jcu::unio::SocketConnectEvent> callback
  ) override {
    std::shared_ptr<SSLSocketImpl> self(self_.lock());
    parent_->connect(connect_param, [self, callback = std::move(callback), connect_param](jcu::unio::SocketConnectEvent& event, jcu::unio::Resource& handle) mutable -> void {
      if (event.hasError()) {
        callback(event, *self);
        return ;
      }

      self->connect_event_ = std::move(callback);

      self->ssl_engine_ = std::move(self->ssl_context_->createEngine(kSslClient));
      const char* hostname = connect_param->getHostname();
      if (hostname) self->ssl_engine_->setHostname(hostname);
      self->ssl_engine_->beginHandshake();

      auto buffer = createFixedSizeBuffer(8192);
      self->parent_->read(buffer, [self](SocketReadEvent& event, Resource& handle) -> void {
        if (event.hasError()) {
          if (self->read_event_callback_) {
            self->read_event_callback_(event, *self);
          }
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
              self->read_event_callback_(event, *self);
            }
          }
        } while(inbound_buffer && (result & SSLEngine::kDataReadMore));
        self->tlsProcess();
      });

      self->tlsProcess();
    });
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
          connect_event_(event, *this);
          connect_event_ = nullptr;
        }
        break;
      case SSLEngine::kHandshakeClosing:
        close();
        break;
      case SSLEngine::kHandshakeFailed:
        if (connect_event_) {
          SocketConnectEvent event { ssl_engine_->getHandshakeError() };
          connect_event_(event, *this);
          connect_event_ = nullptr;
        }
    }
  }

  void disconnect(CompletionOnceCallback<SocketDisconnectEvent> callback) override {
    cancelRead();
    if (ssl_engine_->shutdown()) {
      tlsProcess();
    }
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
//    parent_ = nullptr;

    CloseEvent event;
    emit<CloseEvent>(event);
  }

  void close() override {
    closeImpl(false);
  }
};

std::shared_ptr<SSLSocket> SSLSocket::create(
    std::shared_ptr<Loop> loop,
    std::shared_ptr<Logger> log,
    std::shared_ptr<SSLContext> ssl_context
) {
  std::shared_ptr<SSLSocketImpl> instance(new SSLSocketImpl(std::move(loop), std::move(log), std::move(ssl_context)));
  instance->self_ = instance;
  return std::move(instance);
}

} // namespace unio
} // namespace jcu
