#include <jcu-unio/loop.h>
#include <jcu-unio/log.h>
#include <jcu-unio/net/tcp_socket.h>

#include <thread>
#include <utility>

// 52.43.121.77
using namespace ::jcu::unio;

class EchoClient {
 private:
  std::shared_ptr<Loop> loop_;
  std::shared_ptr<Logger> log_;

  std::weak_ptr<EchoClient> self_;
  std::shared_ptr<TCPSocket> socket_;
  std::shared_ptr<Buffer> read_buf_;
  std::shared_ptr<Buffer> write_buf_;

  bool is_first_send_;

  EchoClient(std::shared_ptr<Loop> loop, std::shared_ptr<Logger> log) :
      loop_(std::move(loop)), log_(std::move(log)), is_first_send_(false)
  {
    log_->logf(jcu::unio::Logger::kLogTrace, "EchoClient: construct");
  }

  // channelActive
  // channelRead
  // channelReadComplete

  void init() {
    socket_ = TCPSocket::create(loop_, log_);
    read_buf_ = createFixedSizeBuffer(1024);
    write_buf_ = createFixedSizeBuffer(1024);
    socket_->on<CloseEvent>([](CloseEvent& event, TCPSocket& handle) -> void {
      printf("CloseEvent\n");
    });
  }

 public:
  ~EchoClient() {
    log_->logf(jcu::unio::Logger::kLogTrace, "EchoClient: destruct");
  }

  static std::shared_ptr<EchoClient> create(std::shared_ptr<Loop> loop, std::shared_ptr<Logger> log) {
    std::shared_ptr<EchoClient> instance(new EchoClient(std::move(loop), std::move(log)));
    instance->self_ = instance;
    instance->init();
    return instance;
  }

  void connect(std::shared_ptr<ConnectParam> connect_param) {
    std::shared_ptr<EchoClient> self(self_.lock());
    socket_->connect(connect_param, [self](SocketConnectEvent& event, TCPSocket& handle) -> void {
      printf("CONNECTED %d %s // %d\n", event.error().code(), event.error().what(), handle.isConnected());
      if (event.hasError()) {
        handle.close();
        return ;
      }
      handle.read(self->read_buf_, [self](SocketReadEvent& event, TCPSocket& handle) -> void {
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
            handle.write(self->write_buf_, [](SocketWriteEvent &event, TCPSocket &handle) -> void {
              fprintf(stderr, "WRITE err=%d / %d / %s\n", event.hasError(), event.error().code(), event.error().what());
            });
          } else {
            handle.disconnect([](auto &event, auto &handle) -> void {
              printf("DISCONNECTED\n");
              handle.close();
            });
          }
        }
      });
    });
  }
};


int main() {
  setbuf(stdout, 0);
  setbuf(stderr, 0);

  std::shared_ptr<Logger> log = createDefaultLogger([](const std::string& str) -> void {
    fprintf(stderr, "%s\n", str.c_str());
  });
  std::shared_ptr<Loop> loop = UnsafeLoop::fromDefault();
  std::shared_ptr<EchoClient> echo_client(EchoClient::create(loop, log));

  auto addr_param = std::make_shared<SockAddrConnectParam<sockaddr_in>>();
  uv_ip4_addr("52.43.121.77", 9000, addr_param->getSockAddr());
//  uv_ip4_addr("127.0.0.1", 7777, addr_param->getSockAddr());

  echo_client->connect(addr_param);

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
