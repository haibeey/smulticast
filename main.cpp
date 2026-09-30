

#include <spdlog/fmt/bin_to_hex.h>

#include "broadcast.hpp"
#include "config.hpp"
#include "spdlog/spdlog.h"

int main(int argc, char *argv[]) {

  if (argc < 2) {
    spdlog::error("usage: {} <config.ini> [threads]", argv[0]);
    return 1;
  }

  std::string config_path = argv[1];
  int num_threads = argc > 2 ? std::stoi(argv[2]) : 2;

  smulticast::Broadcast b(config_path, num_threads);
  if (b.setup() < 0){
      spdlog::error("[main.cpp:21] Setup failed ");
      return ERR;
  };

  std::array<std::byte, BUFSIZE> buffer;
  ssize_t recv_bytes = 0;

  while (recv_bytes != ERR) {
    spdlog::info("[main.cpp:29] Waiting for requests!!!");
    recv_bytes = b.run(buffer);

    if (recv_bytes > 0){
        spdlog::debug(
            "[main.cpp:34] recv {} bytes: {:a}", recv_bytes,
            spdlog::to_hex(buffer.begin(), buffer.begin() + recv_bytes));
    }

  }

  return OK;
}
