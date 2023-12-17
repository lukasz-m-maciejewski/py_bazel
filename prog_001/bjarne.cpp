#include <format>
#include <iostream>
#include <mutex>
#include <ranges>
#include <thread>
#include <vector>

int main() {
  std::mutex out_stream;

  std::vector<std::thread> threads;
  for (auto i = 0; i < 64; ++i) {
    threads.emplace_back([i, &out_stream] {
      std::lock_guard l{out_stream};
      std::cout << std::format("Thread {}\n", i);
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  return 0;
}
