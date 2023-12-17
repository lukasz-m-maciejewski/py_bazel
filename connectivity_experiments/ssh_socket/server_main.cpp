#include "connectivity_experiments/ssh_socket/server_main.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "connectivity_experiments/ssh_socket/server_print.hpp"
#include "third_party/thread_pool/thread_pool.hpp"

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
