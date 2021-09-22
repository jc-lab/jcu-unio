/**
 * @file	openssl_provider.cc
 * @author	Joseph Lee <joseph@jc-lab.net>
 * @date	2021-09-14
 * @copyright Copyright (C) 2021 jc-lab. All rights reserved.
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */

#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/evp.h>
#include <openssl/kdf.h>

#include <jcu-unio/log.h>
#include <jcu-unio/buffer.h>
#include <jcu-unio/net/openssl_provider.h>

namespace jcu {
namespace unio {
namespace openssl {

void SslDeleter::operator()(SSL *ptr) {
  SSL_free(ptr);
}

void SslCtxDeleter::operator()(SSL_CTX *ptr) {
  SSL_CTX_free(ptr);
}

void BioDeleter::operator()(BIO *ptr) {
  BIO_free_all(ptr);
}

class OpenSSLEngineImpl : public OpenSSLEngine {
 public:
  enum State {
    kStateClosed,
    kStateHandshaking,
    kStateHandshaked,
    kStateClosing,
    kStateError
  };

 private:
  std::shared_ptr<OpenSSLContext> ssl_context_;
  std::shared_ptr<Logger> logger_;

  SSLRole role_;

  unique_bio_t err_bio_;
  unique_ssl_t ssl_;
  unique_bio_t app_bio_;

  std::string hostname_;

  State state_;
  std::shared_ptr<SslErrorEvent> handshake_error_;

 public:
  OpenSSLEngineImpl(std::shared_ptr<OpenSSLContext> ssl_context, SSLRole role) :
      ssl_context_(ssl_context),
      role_(role),
      state_(kStateClosed),
      handshake_error_(nullptr) {
    logger_ = createDefaultLogger([](const std::string &msg) -> void {
      fprintf(stderr, "%s\n", msg.c_str());
    });
    err_bio_.reset(BIO_new(BIO_s_mem()));
  }

  bool handleError(int rv, bool force_fatal = false) {
    long msg_size = 0;
    char *msg_ptr = nullptr;
    std::string message;

    if (rv > 0) {
      return false;
    }

    if (force_fatal) {
      //TODO: ERROR

      if (err_bio_) {
        ERR_print_errors(err_bio_.get());
        msg_size = BIO_get_mem_data(err_bio_.get(), &msg_ptr);
        if (msg_size > 0) {
          message = std::string(msg_ptr, msg_size);
        }
      }

      state_ = kStateError;
      handshake_error_ = std::make_shared<SslErrorEvent>(rv, message);
      return true;
    }

    switch (SSL_get_error(ssl_.get(), rv)) {
      case SSL_ERROR_NONE: //0
      case SSL_ERROR_SSL:  // 1
        if (err_bio_) {
          ERR_print_errors(err_bio_.get());
          msg_size = BIO_get_mem_data(err_bio_.get(), &msg_ptr);
          if (msg_size > 0) {
            message = std::string(msg_ptr, msg_size);
          }
          logger_->logf(Logger::kLogWarn, "SSL ERROR: %s", message.c_str());
        }

      case SSL_ERROR_WANT_READ: // 2
      case SSL_ERROR_WANT_WRITE: // 3
      case SSL_ERROR_WANT_X509_LOOKUP:  // 4
//        tlsProcess();
        break;
      case SSL_ERROR_ZERO_RETURN: // 5
      case SSL_ERROR_SYSCALL: //6
      case SSL_ERROR_WANT_CONNECT: //7
      case SSL_ERROR_WANT_ACCEPT: //8
        if (err_bio_) {
          ERR_print_errors(err_bio_.get());
          msg_size = BIO_get_mem_data(err_bio_.get(), &msg_ptr);
          if (msg_size > 0) {
            message = std::string(msg_ptr, msg_size);
          }

          state_ = kStateError;
          handshake_error_ = std::make_shared<SslErrorEvent>(rv, message);
        }
    }

    return rv < 0;
  }

//  void tlsProcess() {
//    int pending = BIO_pending(app_bio_.get());
//    fprintf(stderr, "tlsProcess: BIO_pending: %d\n", pending);
//    if (pending > 0) {
//      handshake_status_ = kHandshakeNeedWrap;
//    } else {
//      OSSL_HANDSHAKE_STATE state = SSL_get_state(ssl_.get());
//      fprintf(stderr, "SSL_get_state: %d\n", state);
//      if (state == TLS_ST_OK) {
//        handshake_status_ = kHandshakeFinished;
//      } else {
//        handshake_status_ = kHandshakeNeedUnwrap;
//      }
//    }
//  }

  HandshakeStatus getHandshakeStatus() override {
    if (state_ == kStateHandshaked) {
      return kHandshakeFinished;
    }
    int pending = BIO_pending(app_bio_.get());
    if (pending > 0) {
      return kHandshakeNeedWrap;
    }
    if (state_ == kStateClosing) {
      return kHandshakeClosing;
    }
    return kHandshakeNeedUnwrap;
  }

  std::shared_ptr<SslErrorEvent> getHandshakeError() const override {
    return handshake_error_;
  }

  void setHostname(const std::string &hostname) {
    hostname_ = hostname;
  }

  void beginHandshake() override {
    BIO *ssl_bio = nullptr;
    BIO *app_bio = nullptr;

    ssl_.reset(SSL_new(ssl_context_->getNativeCtx()));
    if (!hostname_.empty()) {
      SSL_set_tlsext_host_name(ssl_.get(), hostname_.c_str());
    }

    if (role_ == kSslServer) {
      SSL_set_accept_state(ssl_.get());
    } else {
      SSL_set_connect_state(ssl_.get());
    }

    //use default buf size for now.
    if (!BIO_new_bio_pair(&ssl_bio, 0, &app_bio, 0)) {
      //TODO: ERROR
      handleError(-1, true);
      return;
    }

    app_bio_.reset(app_bio);

    SSL_set_bio(ssl_.get(), ssl_bio, ssl_bio);

    doHandshake();
  }

  int doHandshake() {
    int rv = 0;
    rv = SSL_do_handshake(ssl_.get());
    if (handleError(rv, false)) {
      return rv;
    }

    if (rv == 1) {
      state_ = kStateHandshaked;
    }

    return rv;
  }

  DataResult wrap(Buffer *input, Buffer *output) override {
    int rv = 0;
    if (input) {
      rv = SSL_write(ssl_.get(), input->data(), input->remaining());
      if (handleError(rv)) {
        return kDataClosed;
      }
      input->position(input->position() + rv);
    }
    if (output) {
      int pending = BIO_pending(app_bio_.get());
      if (pending > 0) {
        rv = BIO_read(app_bio_.get(), output->data(), output->remaining());
        if (handleError(rv)) {
          return kDataClosed;
        }
      }
      output->position(output->position() + rv);
      output->flip();
    }
    return kDataOk;
  }

  DataResult unwrap(Buffer *input, Buffer *output) override {
    int rv = 0;
    if (input && input->remaining() > 0) {
      rv = BIO_write(app_bio_.get(), input->data(), input->remaining());
      if (handleError(rv)) {
        return kDataClosed;
      }
      input->position(input->position() + rv);
    }
    if (!SSL_is_init_finished(ssl_.get())) {
      if (doHandshake() != 1) {
        return kDataOk;
      }
    }
    if (output) {
      rv = SSL_read(ssl_.get(), output->data(), output->remaining());
      if (handleError(rv)) {
        return kDataClosed;
      }
      output->position(output->position() + rv);
      output->flip();

      rv = SSL_pending(ssl_.get());
      if (rv > 0) {
        return (DataResult)(kDataRead | kDataReadMore);
      }
      return kDataRead;
    }
    return kDataOk;
  }

  bool shutdown() override {
    if (state_ != kStateHandshaked) return false;
    int rv = SSL_shutdown(ssl_.get());
    if (handleError(rv)) {
      return false;
    }
    state_ = kStateClosing;
    return true;
  }
};

class OpenSSLContextImpl : public OpenSSLContext {
 public:
  std::weak_ptr<OpenSSLContextImpl> self_;

  std::shared_ptr<OpenSSLProvider> provider_;

  unique_ssl_ctx_t ssl_ctx_;

  ~OpenSSLContextImpl() override {

  }

  OpenSSLContextImpl(std::shared_ptr<OpenSSLProvider> provider, const SSL_METHOD *method) :
      provider_(provider) {
    ssl_ctx_.reset(SSL_CTX_new(method));
  }

  SSL_CTX *getNativeCtx() override {
    return ssl_ctx_.get();
  }

  std::shared_ptr<SSLProvider> getProvider() const override {
    return provider_;
  }

  std::unique_ptr<SSLEngine> createEngine(SSLRole role) const override {
    return std::make_unique<OpenSSLEngineImpl>(self_.lock(), role);
  }
};

class OpenSSLProviderImpl : public OpenSSLProvider {
 public:
  std::weak_ptr<OpenSSLProviderImpl> self_;

  std::shared_ptr<OpenSSLContext> createOpenSSLContext(const SSL_METHOD *method) const override {
    auto instance = std::make_shared<OpenSSLContextImpl>(self_.lock(), method);
    instance->self_ = instance;
    return std::move(instance);
  }

  bool tls1Prf(const uint8_t *seed,
               int seed_len,
               const uint8_t *secret,
               int secret_len,
               uint8_t *output,
               int output_len) const override {
    bool ret = false;
    EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_TLS1_PRF, nullptr);
    if (!pctx) {
      return false;
    }
    do {
      if (!EVP_PKEY_derive_init(pctx)) {
        break;
      }
      if (!EVP_PKEY_CTX_set_tls1_prf_md(pctx, EVP_md5_sha1())) {
        break;
      }
      if (!EVP_PKEY_CTX_set1_tls1_prf_secret(pctx, secret, secret_len)) {
        break;
      }
      if (!EVP_PKEY_CTX_add1_tls1_prf_seed(pctx, seed, seed_len)) {
        break;
      }
      size_t out_len = output_len;
      if (!EVP_PKEY_derive(pctx, output, &out_len)) {
        break;
      }
      if (out_len != output_len) {
        break;
      }
      ret = true;
    } while (0);
    EVP_PKEY_CTX_free(pctx);
    return ret;
  }

};

std::shared_ptr<OpenSSLProvider> OpenSSLProvider::create() {
  std::shared_ptr<OpenSSLProviderImpl> instance(new OpenSSLProviderImpl());
  instance->self_ = instance;
  return std::move(instance);
}

} // namespace openssl
} // namespace unio
} // namespace jcu
