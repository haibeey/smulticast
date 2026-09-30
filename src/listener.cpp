
#include <arpa/inet.h>
#include <netinet/in.h>
#include <spdlog/spdlog.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <utility>

#include "config.hpp"
#include "listener.hpp"

smulticast::Listener::Listener(int port) : port(port) {};
smulticast::Listener::Listener() : Listener(DEFAULT_PORT) {}

[[nodiscard]] int smulticast::Listener::bind() {

  if (sock_fd != -1) {
    return sock_fd;
  }

  if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    spdlog::error(
        "[listener.cpp:24] Failed to initiate a connection on a socket");
    return ERR;
  }

  int option = 1;
  if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &option,
                 sizeof(option))) {
    spdlog::error("[listener.cpp:31] Failed to make socket re usable");
    return ERR;
  }

  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  if (::bind(sock_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
    spdlog::error("[listener.cpp:40] Failed to bind socket");
    return ERR;
  }

  if (::listen(sock_fd, SOMAXCONN) < 0) {
    spdlog::error("[listener.cpp:45] Failed to listen socket");
    return ERR;
  }

  return sock_fd;
}

smulticast::Listener::Listener(Listener &&l) noexcept
    : sock_fd(std::exchange(l.sock_fd, -1)) {}

smulticast::Listener::~Listener() {
  if (sock_fd >= 0)
    ::close(sock_fd);
}

void smulticast::Listener::Close(int send_sock) {
  if (send_sock != -1 && send_sock != sock_fd)
    ::close(send_sock);
}

smulticast::Listener &
smulticast::Listener::operator=(smulticast::Listener &&l) noexcept {
  if (this != &l) {
    if (sock_fd != -1) {
      ::close(sock_fd);
    }
    sock_fd = l.sock_fd;
    port = l.port;
    addr = l.addr;

    l.sock_fd = -1;
  }
  return *this;
};

[[nodiscard]] int smulticast::Listener::accept() {
  int sock_fd = bind();
  if (sock_fd < 0) {
    spdlog::error("[listener.cpp:84] Failed bind");
    return ERR;
  }
  int incoming_sock = -1;
  socklen_t addrlen = sizeof(addr);
  if ((incoming_sock = ::accept(
                          sock_fd, reinterpret_cast<struct sockaddr *>(&addr),
                          &addrlen)) < 0) {
    spdlog::error("[listener.cpp:87] Failed to accept connection");
    return ERR;
  }
  return incoming_sock;
}
