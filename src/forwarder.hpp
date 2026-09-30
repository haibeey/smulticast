#pragma once


#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>


namespace smulticast {
class Forwarder {
private:
  int sock_fd = -1;
  int port;
  std::string host;
  struct sockaddr_in addr;

public:
  Forwarder(std::string host,int port);
  Forwarder(std::string host);
  Forwarder(int port);
  Forwarder();
  [[nodiscard]] int connect();
  ~Forwarder();
  Forwarder(Forwarder &&f) noexcept;
  Forwarder& operator=(Forwarder &&l) noexcept;
  Forwarder(const Forwarder&) = delete;

};
}
