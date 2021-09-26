#include <jcu-unio/loop.h>
#include <jcu-unio/log.h>
#include <jcu-unio/net/tcp_socket.h>
#include <jcu-unio/net/ssl_socket.h>
#include <jcu-unio/net/openssl_provider.h>

#include <thread>
#include <utility>

// 52.43.121.77
using namespace ::jcu::unio;

class SimpleHttpsClient {
 private:
  std::shared_ptr<Loop> loop_;
  std::shared_ptr<Logger> log_;

  std::weak_ptr<SimpleHttpsClient> self_;
  std::shared_ptr<SSLContext> ssl_context_;

  std::shared_ptr<SSLSocket> socket_;
  std::shared_ptr<Buffer> read_buf_;
  std::shared_ptr<Buffer> write_buf_;

  SimpleHttpsClient(std::shared_ptr<Loop> loop, std::shared_ptr<Logger> log, std::shared_ptr<SSLContext> ssl_context) :
      loop_(std::move(loop)), log_(std::move(log)), ssl_context_(std::move(ssl_context))
  {
    log_->logf(jcu::unio::Logger::kLogTrace, "SimpleHttpsClient: construct");
  }

  // channelActive
  // channelRead
  // channelReadComplete

  void init() {
    std::shared_ptr<SimpleHttpsClient> self(self_.lock());
    jcu::unio::BasicParams basic_params { loop_, log_ };
    auto tcp_socket = TCPSocket::create(basic_params);
    socket_ = SSLSocket::create(basic_params, ssl_context_);
    socket_->setParent(tcp_socket);

    read_buf_ = createFixedSizeBuffer(1024);
    write_buf_ = createFixedSizeBuffer(1024);
    socket_->once<CloseEvent>([](CloseEvent &event, Resource &handle) -> void {
      fprintf(stderr,"SSLSocket CloseEvent\n");
    });
    socket_->on<SocketReadEvent>([self](SocketReadEvent &event, Resource &base_handle) -> void {
      SSLSocket &handle = dynamic_cast<SSLSocket &>(base_handle);

      if (event.hasError()) {
        auto &error = event.error();
        printf("READ ERR: %d %s\n", error.code(), error.what());

        handle.close();
      } else {
        std::string text((const char *) event.buffer()->data(), event.buffer()->remaining());
        printf("READ: %s\n", text.c_str());

        self->loop_->sendQueuedTask([handle = handle.shared()]() -> void {
          fprintf(stderr, "GO DISCONNECT\n");
          handle->disconnect([](auto &event, auto &handle) -> void {
            fprintf(stderr, "DISCONNECTED\n");
            if (event.hasError()) {
              fprintf(stderr, "DISCONNECT FAILED: %d: %s\n", event.error().code(), event.error().what());
            } else {
              handle.close();
            }
          });
        });
      }
    });
  }

 public:
  ~SimpleHttpsClient() {
    log_->logf(jcu::unio::Logger::kLogTrace, "SimpleHttpsClient: destruct");
  }

  static std::shared_ptr<SimpleHttpsClient> create(std::shared_ptr<Loop> loop, std::shared_ptr<Logger> log, std::shared_ptr<SSLContext> ssl_context) {
    std::shared_ptr<SimpleHttpsClient> instance(new SimpleHttpsClient(std::move(loop), std::move(log), std::move(ssl_context)));
    instance->self_ = instance;
    instance->init();
    return instance;
  }

  void write(const std::string& write_data) {
    memcpy(write_buf_->data(), write_data.c_str(), write_data.size());
    write_buf_->position(write_data.size());
    write_buf_->flip();
    socket_->write(write_buf_, [](SocketWriteEvent& event, auto& handle) -> void {
      fprintf(stderr, "WRITE OK\n");
    });
  }

  void start(std::shared_ptr<ConnectParam> connect_param) {
    std::shared_ptr<SimpleHttpsClient> self(self_.lock());
    socket_->once<jcu::unio::InitEvent>([self, connect_param](InitEvent& event, auto& resource) -> void {
      auto& socket = dynamic_cast<jcu::unio::SSLSocket&>(resource);
      socket.connect(connect_param, [self](SocketConnectEvent& event, Resource& base_handle) -> void {
        SSLSocket &handle = dynamic_cast<SSLSocket &>(base_handle);

        if (event.hasError()) {
          fprintf(stderr, "CONNECT FAILED: %d, %s\n", event.error().code(), event.error().what());
          handle.close();
          return;
        } else {
          fprintf(stderr, "CONNECTED\n");
        }

        handle.read(self->read_buf_);
        self->write("GET / HTTP/1.1\nHost: api.myip.com\n\n");
      });
    });
  }
};


int main() {
  setbuf(stdout, 0);
  setbuf(stderr, 0);

  std::shared_ptr<Logger> log = createDefaultLogger([](jcu::unio::Logger::LogLevel level, const std::string& str) -> void {
    fprintf(stderr, "%s\n", str.c_str());
  });
  std::shared_ptr<Loop> loop = UnsafeLoop::fromDefault();
  std::shared_ptr<openssl::OpenSSLProvider> openssl_provider(openssl::OpenSSLProvider::create());
  std::shared_ptr<openssl::OpenSSLContext> openssl_context(openssl_provider->createOpenSSLContext(TLSv1_2_method()));

  std::shared_ptr<SimpleHttpsClient> echo_client(SimpleHttpsClient::create(loop, log, openssl_context));

  auto addr_param = std::make_shared<SockAddrConnectParam<sockaddr_in>>();
  addr_param->setHostname("api.myip.com");
  uv_ip4_addr("172.67.208.45", 443, addr_param->getSockAddr());

  echo_client->start(addr_param);

  loop->init();

  std::thread th([loop]() -> void {
    std::this_thread::sleep_for(std::chrono::milliseconds { 5000 });
    loop->sendQueuedTask([loop]() -> void {
      loop->uninit();
    });
  });

  uv_run(loop->get(), UV_RUN_DEFAULT);

  th.join();

  return 0;
}
