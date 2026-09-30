
#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>


namespace smulticast {
class Listener {
private:
  int sock_fd = -1;
  int port;
  struct sockaddr_in addr;
  [[nodiscard]] int bind();

public:
  Listener(int port);
  Listener();
  [[nodiscard]] int accept();
  void Close(int send_sock);
  ~Listener();
  Listener(Listener &&l) noexcept;
  Listener& operator=(Listener &&l) noexcept;
  Listener(const Listener&) = delete;
};
}
