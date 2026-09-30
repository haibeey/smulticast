
#include <algorithm>
#include <array>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <memory>
#include <mutex>
#include <set>
#include <span>
#include <spdlog/spdlog.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

#include "broadcast.hpp"
#include "config.hpp"
#include "forwarder.hpp"
#include "inicpp.h"
#include "listener.hpp"

smulticast::Handler::Handler() : id(++next_id), alive(false) {};

void smulticast::Handler::kill() {
  {
    std::lock_guard<std::mutex> lock(mtx);
    alive = false;
  }
  cv.notify_one();
}

void smulticast::Handler::join() {
  if (work.joinable())
    work.join();
}

void smulticast::Handler::handle(const std::span<std::byte> buffer) {
  std::lock_guard<std::mutex> lock(mtx);
  bool empty = queue.empty();
  queue.push_front(std::vector<std::byte>(buffer.begin(), buffer.end()));
  if (empty) {
    cv.notify_one();
  }
}

void smulticast::Handler::do_work() {
  std::vector<int> forwaders_sock;
  int forward_sock;
  for (Forwarder &forwader : forwarders) {
    forward_sock = forwader.connect();
    if (forward_sock >= 0) {
      forwaders_sock.push_back(forward_sock);
    } else {
    }
  }

  if (forwaders_sock.empty()) {
    spdlog::warn("[broadcast.cpp:72] Thread {}, could not connect. stopping",
                 id);
    return;
  }

  if (forwaders_sock.size() != forwarders.size()) {
    spdlog::warn("[broadcast.cpp:78] Some connection were unsuccesful");
  }

  while (alive) {
    std::unique_lock<std::mutex> lock(mtx);
    if (alive && queue.empty()) {
      cv.wait(lock, [this] { return !alive || !queue.empty(); });
    }
    lock.unlock();

    if (!alive)
      return;

    std::vector<std::byte> buffer;
    {
      std::lock_guard<std::mutex> lock(mtx);
      buffer = std::move(queue.back());
      queue.pop_back();
    }

    ssize_t buf_size = buffer.size();
    ssize_t cursor = 0;
    ssize_t sent, to_send;
    ssize_t sock_cursor = 0, sock_to_send;

    std::set<int> dead_socks;

    while (buf_size > 0) {
      to_send = std::min<ssize_t>(buf_size, BUFSIZE);

      for (int &sock_id : forwaders_sock) {

        sent = ::send(sock_id, buffer.data() + cursor, to_send, 0);
        if (sent <= 0) {
          spdlog::warn("[broadcast.cpp:109] Thread {}, socket {} disconnected "
                       ", buf left is {}",
                       id, sock_id, buf_size);
          dead_socks.emplace(sock_id);
        } else {

          // We sent data partially for this sock_id so let's try to finish
          // sending the data.

          sock_cursor = cursor + sent;
          sock_to_send = to_send - sent;

          while (sent > 0 && sock_to_send > 0) {

            sent =
                ::send(sock_id, buffer.data() + sock_cursor, sock_to_send, 0);
            if (sent <= 0) {
              spdlog::warn("[broadcast.cpp:127] Thread {}, socket {} "
                           "disconnected , buf left is {}",
                           id, sock_id, buf_size);
              dead_socks.emplace(sock_id);
              break;
            }

            sock_to_send -= sent;
            sock_cursor += sent;
          }
        }
      }

      cursor += to_send;
      buf_size -= to_send;
    }

    std::erase_if(forwaders_sock, [dead_socks](int sock_id) {
      return dead_socks.find(sock_id) != dead_socks.end();
    });
  }
}

void smulticast::Handler::start_work() {
  alive = true;
  work = std::thread(&Handler::do_work, this);
}

void smulticast::Handler::set_forwaders(
    std::vector<smulticast::Forwarder> &fwds) {
  forwarders = std::move(fwds);
}

std::vector<std::vector<smulticast::Forwarder>>
smulticast::Broadcast::get_forward_address(std::string config_file_path) {
  std::vector<std::vector<smulticast::Forwarder>> addresses(num_threads);
  ini::IniFile inif(config_file_path);

  const auto &smulticast_section = inif["smulticast"];

  int pos = 0;
  for (const auto &domain_ip : smulticast_section) {
    std::string domain = domain_ip.first;
    std::string ip_port = domain_ip.second.as<std::string>();

    size_t colon_pos = ip_port.find(':');

    std::string ip = ip_port.substr(0, colon_pos);
    std::string port = colon_pos == std::string::npos
                           ? DEFAULT_HOST_PORT
                           : ip_port.substr(colon_pos + 1);

    addresses[++pos % num_threads].push_back({ip, std::stoi(port)});
  }

  return addresses;
}

smulticast::Broadcast::Broadcast(std::string config_path, int num_threads)
    : config_path(std::move(config_path)), num_threads(num_threads) {

  slis = smulticast::Listener();
  for (int i = 0; i < num_threads; ++i)
    handlers.push_back(std::make_unique<Handler>());
};

int smulticast::Broadcast::setup() {
  spdlog::info("[broadcast.cpp:183] Just setting up the broadcast");

  std::vector<std::vector<smulticast::Forwarder>> fwds =
      get_forward_address(config_path);

  spdlog::info("[broadcast.cpp:188] Listening for a connection !!!");
  if (listener_sock == -1 && (listener_sock = slis.accept()) < 0) {
    spdlog::error("[broadcast.cpp:189] Connection Failed. sock id is {}",
                  listener_sock);
    return ERR;
  }

  spdlog::info("[broadcast.cpp:190] Connection accepted. sock id is {}",
               listener_sock);

  int pos = 0;
  for (std::unique_ptr<Handler> &handler : handlers) {
    handler->set_forwaders(fwds[pos++]);
    handler->start_work();
  }
  return OK;
}

ssize_t smulticast::Broadcast::run(std::array<std::byte, BUFSIZE> &buffer) {

  ssize_t recv_bytes = ::recv(listener_sock, buffer.data(), BUFSIZE, 0);
  if (recv_bytes <= 0)
    return ERR;
  for (std::unique_ptr<Handler> &handler : handlers) {
    handler->handle(std::span<std::byte>(buffer.data(), recv_bytes));
  }

  return recv_bytes;
}

smulticast::Broadcast::~Broadcast() {

  for (std::unique_ptr<Handler> &handler : handlers) {
    handler->kill();
  }

  for (std::unique_ptr<Handler> &handler : handlers) {
    handler->join();
  }
}
