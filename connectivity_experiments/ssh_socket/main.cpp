// Stuff

#include <fmt/core.h>

#include <chrono>
#include <thread>

#include "connectivity_experiments/ssh_socket/client_main.hpp"
#include "connectivity_experiments/ssh_socket/server_main.hpp"

int main() {
  fmt::print("It lives\n");

  std::thread server{server_main};
  std::this_thread::sleep_for(std::chrono::milliseconds{100});

  std::thread client{client_main};

  client.join();
  server.join();

  return 0;
}
