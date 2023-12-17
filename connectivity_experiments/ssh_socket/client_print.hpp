#pragma once

#include <utility>

#include "connectivity_experiments/ssh_socket/safe_print.hpp"

template <typename... Args>
void client_print(std::string_view msg, Args... args) {
  safe_print(
      fmt::format("client> {}",
                  fmt::format(fmt::runtime(msg), std::forward<Args>(args)...)));
}
