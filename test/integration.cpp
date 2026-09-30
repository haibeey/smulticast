
#include <gtest/gtest.h>

#include <array>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "broadcast.hpp"
#include "config.hpp"

using namespace std::chrono_literals;


static FILE *start_receiver(int port) {
  std::string cmd = "nc -l " + std::to_string(port);
  return popen(cmd.c_str(), "r");
}

static std::string read_all(FILE *pipe) {
  std::string out;
  std::array<char, 256> chunk;
  size_t n;
  while ((n = fread(chunk.data(), 1, chunk.size(), pipe)) > 0)
    out.append(chunk.data(), n);
  pclose(pipe);
  return out;
}

// Connects to the broadcaster, retrying until it is listening.
static int connect_sender(int port) {
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

  for (int i = 0; i < 50; ++i) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (::connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == 0)
      return fd;
    ::close(fd);
    std::this_thread::sleep_for(100ms);
  }
  return -1;
}

TEST(INTEGRATION, receivers_get_sent_data) {
  const int ports[] = {18001, 18002};

  auto config = std::filesystem::temp_directory_path() / "smulticast_test.ini";
  {
    std::ofstream f(config);
    f << "[smulticast]\n"
      << "a = 127.0.0.1:" << ports[0] << "\n"
      << "b = 127.0.0.1:" << ports[1] << "\n";
  }

  FILE *receiver_a = start_receiver(ports[0]);
  FILE *receiver_b = start_receiver(ports[1]);
  ASSERT_NE(receiver_a, nullptr);
  ASSERT_NE(receiver_b, nullptr);
  std::this_thread::sleep_for(300ms); // let nc start listening

  std::thread broadcaster([&config] {
    smulticast::Broadcast b(config.string(), 2);
    if (b.setup() < 0)
      return;
    std::array<std::byte, BUFSIZE> buffer;
    while (b.run(buffer) != ERR) {
    }
  });

  int sender = connect_sender(DEFAULT_PORT);
  ASSERT_GE(sender, 0);

  const std::string message = "hello receivers\n";
  ASSERT_EQ(::send(sender, message.data(), message.size(), 0),
            static_cast<ssize_t>(message.size()));

  std::this_thread::sleep_for(300ms); // give workers time to forward
  ::close(sender);                    // broadcaster sees EOF and shuts down
  broadcaster.join();

  EXPECT_EQ(read_all(receiver_a), message);
  EXPECT_EQ(read_all(receiver_b), message);

  std::filesystem::remove(config);
}
