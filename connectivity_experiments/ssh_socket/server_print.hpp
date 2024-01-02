#pragma once

#include <string_view>
#include <utility>

#include "connectivity_experiments/ssh_socket/safe_print.hpp"

template <typename... Args>
void server_print(std::string_view msg, Args... args) {
  safe_print(
      fmt::format("server> {}",
                  fmt::format(fmt::runtime(msg), std::forward<Args>(args)...)));
}
