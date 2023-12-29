#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <errno.h>
#include <fmt/core.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <mutex>
#include <ratio>
#include <thread>

#include "third_party/thread_pool/thread_pool.hpp"

std::mutex out_mutex;

void safe_print(std::string_view msg) {
  std::lock_guard l{out_mutex};
  fmt::print("{}\n", msg);
}

template <typename... Args>
void server_print(std::string_view msg, Args... args) {
  safe_print(
      fmt::format("server> {}",
                  fmt::format(fmt::runtime(msg), std::forward<Args>(args)...)));
}

template <typename... Args>
void client_print(std::string_view msg, Args... args) {
  safe_print(
      fmt::format("client> {}",
                  fmt::format(fmt::runtime(msg), std::forward<Args>(args)...)));
}

void server_main() {
  server_print("server lives");
  constexpr auto MYPORT = 3490;
  int yes = 1;

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    perror("socket");
    server_print("unable to open socket - bailing out");
    return;
  }

  auto const setsockopt_res =
      setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  if (setsockopt_res == -1) {
    perror("setsockopt");
    server_print("failed to set options - bailing out");
    return;
  }

  sockaddr_in my_addr = [] {
    sockaddr_in addr{};

    addr.sin_family = AF_INET;
    addr.sin_port = htons(MYPORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    return addr;
  }();

  auto const bind_res =
      bind(sockfd, reinterpret_cast<sockaddr*>(&my_addr), sizeof(sockaddr));
  if (bind_res == -1) {
    perror("bind");
    server_print("unable to bind - bailing out");
    return;
  }

  auto constexpr BACKLOG = 10;
  auto const listen_res = listen(sockfd, BACKLOG);
  if (listen_res == -1) {
    perror("listen");
    server_print("listen error - bailing out");
    return;
  }

  BS::thread_pool pool(10);

  std::atomic_bool server_running = true;

  while (server_running.load()) {
    socklen_t sin_size = sizeof(sockaddr_in);
    sockaddr_in their_addr;
    int new_fd =
        accept(sockfd, reinterpret_cast<sockaddr*>(&their_addr), &sin_size);
    if (new_fd == -1) {
      perror("accept");
      continue;
    }

    server_print("server: got connection from {}",
                 inet_ntoa(their_addr.sin_addr));

    pool.push_task([new_fd] {
      constexpr auto bufferSize = 100;
      std::array<char, bufferSize> buffer = {'\0'};
      while (true) {
        auto numRead = recv(new_fd, buffer.data(), bufferSize - 1, 0);
        if (numRead == 0) {
          server_print("exit through numRead == 0");
          std::string const msg = "bye bye";
          send(new_fd, msg.data(), msg.size(), 0);
          break;
        }

        if (numRead == -1) {
          perror("recv");
          break;
        }

        server_print("Received: {}", buffer.data());
        std::string msg = "Thanks for all the fish!\n";
        auto numSent = send(new_fd, msg.data(), msg.size(), 0);
        if (numSent == -1) {
          perror("send");
          break;
        }

        server_print("another iteration");
      }

      server_print("closing new_fd");

      close(new_fd);
    });

    server_print("server main loop ding");
    server_running.store(false);
  }

  server_print("waiting for pool done");
  pool.wait_for_tasks();
  server_print("done");
}

void client_main() {
  client_print("client lives");
  constexpr auto PORT = 3490;
  hostent* he = gethostbyname("localhost");

  if (he == nullptr) {
    perror("gethostbyname");
    return;
  }

  auto sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    perror("socket");
    return;
  }

  sockaddr_in their_addr;
  their_addr.sin_family = AF_INET;
  their_addr.sin_port = htons(PORT);
  their_addr.sin_addr = *reinterpret_cast<in_addr*>(he->h_addr);
  their_addr.sin_addr = {};

  auto const connect_res = connect(
      sockfd, reinterpret_cast<sockaddr*>(&their_addr), sizeof(sockaddr));
  if (connect_res == -1) {
    perror("client connect");
    return;
  }

  std::string const msg = "Howdy Partner!\n";
  send(sockfd, msg.data(), msg.size(), 0);

  constexpr auto buf_size = 100;
  std::array<char, buf_size> buffer = {'\0'};
  recv(sockfd, buffer.data(), buf_size - 1, 0);
  client_print("received: {}", buffer.data());

  client_print("closing sockfd");
  close(sockfd);

  client_print("done");
}

int main() {
  fmt::print("It lives\n");

  std::thread server{server_main};
  std::this_thread::sleep_for(std::chrono::milliseconds{100});

  std::thread client{client_main};

  client.join();
  server.join();

  return 0;
}
