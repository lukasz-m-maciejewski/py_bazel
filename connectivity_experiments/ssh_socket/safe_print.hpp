#pragma once

#include <fmt/core.h>

#include <mutex>
#include <string_view>

inline std::mutex out_mutex;

inline void safe_print(std::string_view msg) {
  std::lock_guard l{out_mutex};
  fmt::print("{}\n", msg);
}
