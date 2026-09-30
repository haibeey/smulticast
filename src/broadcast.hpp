#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <span>
#include <spdlog/spdlog.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

#include "config.hpp"
#include "forwarder.hpp"
#include "listener.hpp"

namespace smulticast {

class Handler {
private:
  inline static std::atomic<std::uint64_t> next_id{0};

  std::uint64_t id;
  std::mutex mtx;
  std::condition_variable cv;
  std::thread work;
  std::atomic<bool> alive;

  std::deque<std::vector<std::byte>> queue;

  std::vector<Forwarder> forwarders;

public:
  Handler(const Handler &) = delete;
  Handler &operator=(const Handler &) = delete;
  Handler();
  Handler(Handler &&other) = delete;
  Handler &operator=(Handler &&other) = delete;
  void handle(const std::span<std::byte> buffer);
  void do_work();
  void start_work();
  void kill();
  void join();

  void set_forwaders(std::vector<Forwarder> &fwds);
};

class Broadcast {
private:
  std::deque<std::unique_ptr<Handler>> handlers;
  Listener slis;
  std::mutex mtx;

  std::string config_path;
  int num_threads;
  int listener_sock = -1;

  std::vector<std::vector<smulticast::Forwarder>>
  get_forward_address(std::string config_file_path);

public:
  Broadcast(std::string config_path, int num_threads);
  [[nodiscard]] int setup();
  ssize_t run(std::array<std::byte, BUFSIZE> &buffer);
  ~Broadcast();
};

}; // namespace smulticast
