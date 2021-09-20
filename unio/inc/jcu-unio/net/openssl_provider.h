/**
 * @file	openssl_provider.h
 * @author	Joseph Lee <joseph@jc-lab.net>
 * @date	2021-09-14
 * @copyright Copyright (C) 2021 jc-lab. All rights reserved.
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */


#ifndef JCU_UNIO_NET_OPENSSL_PROVIDER_H_
#define JCU_UNIO_NET_OPENSSL_PROVIDER_H_

#include <jcu-unio/jcu-unio-config.h>

#ifdef JCU_UNIO_USE_OPENSSL

#include <memory>

#include <openssl/ssl.h>

#include "ssl_context.h"

namespace jcu {
namespace unio {
namespace openssl {

class OpenSSLProvider;

struct SslDeleter {
  void operator()(SSL* ptr);
};
typedef std::unique_ptr<SSL, SslDeleter> unique_ssl_t;

struct SslCtxDeleter {
  void operator()(SSL_CTX* ptr);
};
typedef std::unique_ptr<SSL_CTX, SslCtxDeleter> unique_ssl_ctx_t;

struct BioDeleter {
  void operator()(BIO* ptr);
};
typedef std::unique_ptr<BIO, BioDeleter> unique_bio_t;

class OpenSSLEngine : public SSLEngine {
};

class OpenSSLContext : public SSLContext {
 public:
  virtual ~OpenSSLContext() = default;
  virtual SSL_CTX* getNativeCtx() = 0;
};

class OpenSSLProvider : public SSLProvider {
 public:
  virtual ~OpenSSLProvider() = default;
  static std::shared_ptr<OpenSSLProvider> create();

  virtual std::shared_ptr<OpenSSLContext> createOpenSSLContext(const SSL_METHOD* method) const = 0;

  std::shared_ptr<SSLContext> createContext() const override {
    return createOpenSSLContext(TLS_method());
  }
};

} // namespace unio
} // namespace unio
} // namespace jcu

#endif // JCU_UNIO_USE_OPENSSL

#endif // JCU_UNIO_NET_OPENSSL_PROVIDER_H_
