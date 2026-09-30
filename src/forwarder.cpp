
#include <cstdio>
#include <spdlog/spdlog.h>
#include <string>

#include "config.hpp"
#include "forwarder.hpp"

smulticast::Forwarder::Forwarder(std::string host, int port)
    : host(host), port(port) {}
smulticast::Forwarder::Forwarder(int port) : Forwarder(DEFAULT_HOST, port) {}
smulticast::Forwarder::Forwarder(std::string host)
    : Forwarder(host, DEFAULT_FORWARD_PORT) {}
smulticast::Forwarder::Forwarder() : Forwarder(DEFAULT_FORWARD_PORT) {}
smulticast::Forwarder::Forwarder(smulticast::Forwarder &&f) noexcept {
  if (this != &f) {
    if (sock_fd != -1) {
      ::close(sock_fd);
    }
    sock_fd = f.sock_fd;
    port = f.port;
    addr = f.addr;
    host = f.host;

    f.sock_fd = -1;
  }
}

[[nodiscard]] int smulticast::Forwarder::connect() {

  if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    std::perror("Unknown error occured");
    return -1;
  }

  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);

  if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
    spdlog::error(
        "[forwader.cpp:35] Failed to convert host address to binary form");
    return -1;
  }

  if (::connect(sock_fd, reinterpret_cast<struct sockaddr *>(&addr),
                sizeof(addr)) < 0) {

    spdlog::error(
        "[forwader.cpp:42] Failed to initiate a connection on a socket");
    return -1;
  }

  int on = 1;
  setsockopt(sock_fd, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on));

  return sock_fd;
}

smulticast::Forwarder &
smulticast::Forwarder::operator=(smulticast::Forwarder &&f) noexcept {
  if (this != &f) {
    if (sock_fd != -1) {
      ::close(sock_fd);
    }
    sock_fd = f.sock_fd;
    port = f.port;
    addr = f.addr;

    f.sock_fd = -1;
  }
  return *this;
};

smulticast::Forwarder::~Forwarder() {

  if (sock_fd >= 0)
    ::close(sock_fd);
}
