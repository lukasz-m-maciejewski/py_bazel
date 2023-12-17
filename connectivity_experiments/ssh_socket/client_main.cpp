#include "connectivity_experiments/ssh_socket/client_main.hpp"

#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>

#include <array>

#include "connectivity_experiments/ssh_socket/client_print.hpp"

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
