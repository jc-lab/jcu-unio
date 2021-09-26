#include <jcu-unio/loop.h>
#include <jcu-unio/log.h>
#include <jcu-unio/net/tcp_socket.h>

#include <thread>
#include <utility>

// 52.43.121.77
using namespace ::jcu::unio;

class SimpleHttpsClient {
 private:
  std::shared_ptr<Loop> loop_;
  std::shared_ptr<Logger> log_;

  std::weak_ptr<SimpleHttpsClient> self_;
  std::shared_ptr<TCPSocket> socket_;
  std::shared_ptr<Buffer> read_buf_;
  std::shared_ptr<Buffer> write_buf_;

  bool is_first_send_;

  SimpleHttpsClient(std::shared_ptr<Loop> loop, std::shared_ptr<Logger> log) :
      loop_(std::move(loop)), log_(std::move(log)), is_first_send_(false)
  {
    log_->logf(jcu::unio::Logger::kLogTrace, "EchoClient: construct");
  }

  // channelActive
  // channelRead
  // channelReadComplete

  void init() {
    std::shared_ptr<SimpleHttpsClient> self(self_.lock());
    jcu::unio::BasicParams basic_params { loop_, log_ };
    socket_ = TCPSocket::create(basic_params);
    read_buf_ = createFixedSizeBuffer(1024);
    write_buf_ = createFixedSizeBuffer(1024);
    socket_->on<CloseEvent>([](CloseEvent& event, Resource& resource) -> void {
      printf("CloseEvent\n");
    });
    socket_->on<SocketReadEvent>([self](SocketReadEvent& event, Resource& resource) -> void {
      auto& handle = dynamic_cast<TCPSocket&>(resource);
      if (event.hasError()) {
        auto& error = event.error();
        printf("READ ERR: %d %s\n", error.code(), error.what());

        handle.close();
      } else {
        std::string text((const char *) event.buffer()->data(), event.buffer()->remaining());
        printf("READ: %s\n", text.c_str());

        if (!self->is_first_send_) {
          self->is_first_send_ = true;
          std::string write_data;
          write_data = "HELLO WORLD!!!\n";
          memcpy(self->write_buf_->data(), write_data.c_str(), write_data.size());
          self->write_buf_->position(write_data.size());
          self->write_buf_->flip();
          handle.write(self->write_buf_, [](SocketWriteEvent &event, auto& handle) -> void {
            if (event.hasError()) {
              fprintf(stderr, "WRITE err=%d / %d / %s\n", event.hasError(), event.error().code(), event.error().what());
            } else {
              fprintf(stderr, "WRITE OK\n");
            }
          });
        } else {
          printf("GO DISCONNECT\n");
          self->loop_->sendQueuedTask([handle = handle.shared()]() -> void {
            handle->disconnect([](auto &event, auto &handle) -> void {
              printf("DISCONNECTED\n");
              handle.close();
            });
          });
        }
      }
    });
  }

 public:
  ~SimpleHttpsClient() {
    log_->logf(jcu::unio::Logger::kLogTrace, "EchoClient: destruct");
  }

  static std::shared_ptr<SimpleHttpsClient> create(std::shared_ptr<Loop> loop, std::shared_ptr<Logger> log) {
    std::shared_ptr<SimpleHttpsClient> instance(new SimpleHttpsClient(std::move(loop), std::move(log)));
    instance->self_ = instance;
    instance->init();
    return instance;
  }

  void start(std::shared_ptr<ConnectParam> connect_param) {
    std::shared_ptr<SimpleHttpsClient> self(self_.lock());
    socket_->once<jcu::unio::InitEvent>([self, connect_param](InitEvent& event, auto& resource) -> void {
      auto& socket = dynamic_cast<jcu::unio::TCPSocket&>(resource);
      socket.connect(connect_param, [self](SocketConnectEvent& event, Resource& resource) -> void {
        auto& handle = dynamic_cast<TCPSocket&>(resource);
        if (event.hasError()) {
          printf("CONNECT FAILED %d %s // %d\n", event.error().code(), event.error().what(), handle.isConnected());
        } else {
          printf("CONNECTED %d\n", handle.isConnected());
        }
        if (event.hasError()) {
          handle.close();
          return ;
        }
        handle.read(self->read_buf_);
      });
    });
  }
};


int main() {
  setbuf(stdout, 0);
  setbuf(stderr, 0);

  std::shared_ptr<Logger> log = createDefaultLogger([](Logger::LogLevel level, const std::string& str) -> void {
    fprintf(stderr, "%s\n", str.c_str());
  });
  std::shared_ptr<Loop> loop = UnsafeLoop::fromDefault();
  std::shared_ptr<SimpleHttpsClient> echo_client(SimpleHttpsClient::create(loop, log));

  auto addr_param = std::make_shared<SockAddrConnectParam<sockaddr_in>>();
  uv_ip4_addr("52.43.121.77", 9000, addr_param->getSockAddr());
//  uv_ip4_addr("127.0.0.1", 7777, addr_param->getSockAddr());

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
