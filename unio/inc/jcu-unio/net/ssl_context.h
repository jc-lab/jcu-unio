/**
 * @file	ssl_context.h
 * @author	Joseph Lee <joseph@jc-lab.net>
 * @date	2021-09-14
 * @copyright Copyright (C) 2021 jc-lab. All rights reserved.
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */

#ifndef JCU_UNIO_NET_SSL_CONTEXT_H_
#define JCU_UNIO_NET_SSL_CONTEXT_H_

#include "../event.h"

namespace jcu {
namespace unio {

class Logger;
class Buffer;

class SslErrorEvent : public ErrorEvent {
 private:
  int code_;
  std::string message_;

 public:
  SslErrorEvent();
  SslErrorEvent(int code, const std::string& message);
  int code() const override;
  const char *what() const override;
};

class HandshakeEvent : public BaseEvent {
 protected:
  SslErrorEvent error_;

 public:
  HandshakeEvent();
  HandshakeEvent(SslErrorEvent error);
  bool hasError() const override;
  ErrorEvent &error() override;
  const ErrorEvent &error() const override;
};

enum SSLRole {
  kSslClient,
  kSslServer
};

class SSLEngine {
 public:
  enum HandshakeStatus {
    /**
     * Initial status
     */
    kHandshakeNotStarted,
    /**
     * Need wrap (outbound)
     */
    kHandshakeNeedWrap,
    /**
     * Need unwrap (inbound)
     */
    kHandshakeNeedUnwrap,
    /**
     * Finished
     */
    kHandshakeFinished,
    /**
     * Shutdown
     */
    kHandshakeClosing,
    /**
     * Failed
     */
    kHandshakeFailed,
  };
  enum DataResult {
    kDataOk = 0,
    kDataClosed = 0x01,
    kDataReadMore = 0x02,
  };

  typedef std::function<void(HandshakeEvent &)> HandshakeCallback;

  virtual ~SSLEngine() = default;
//  virtual void setOutboundBuffer(std::shared_ptr<Buffer> buffer) = 0;
//  virtual void setInboundBuffer(std::shared_ptr<Buffer> buffer) = 0;
  virtual void setHostname(const std::string& hostname) = 0;
  virtual void beginHandshake() = 0;
  virtual HandshakeStatus getHandshakeStatus() = 0;
  virtual std::shared_ptr<SslErrorEvent> getHandshakeError() const = 0;
  /**
   *
   * @param input  input data (must be null during handshaking)
   * @param output outbound data
   * @return
   */
  virtual DataResult wrap(Buffer* input, Buffer* output) = 0;
  virtual DataResult unwrap(Buffer* input, Buffer* output) = 0;

  virtual bool shutdown() = 0;
};

class SSLProvider;

class SSLContext {
 public:
  virtual ~SSLContext() = default;

  virtual std::shared_ptr<SSLProvider> getProvider() const = 0;
  virtual std::unique_ptr<SSLEngine> createEngine(SSLRole role) const = 0;
};

class SSLProvider {
 public:
  virtual ~SSLProvider() = default;

  virtual std::shared_ptr<SSLContext> createContext() const = 0;

  virtual bool tls1Prf(
      const uint8_t *seed, int seed_len,
      const uint8_t *secret, int secret_len,
      uint8_t *output, int output_len
  ) const = 0;
};

} // namespace unio
} // namespace jcu

#endif //JCU_UNIO_NET_SSL_CONTEXT_H_
