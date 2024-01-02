#include "connectivity_experiments/ssh_socket/server_main.hpp"

#include <libssh/callbacks.h>
#include <libssh/libssh.h>
#include <libssh/server.h>

#include "connectivity_experiments/ssh_socket/server_print.hpp"
#include "connectivity_experiments/ssh_socket/ssh.hpp"
#include "third_party/thread_pool/thread_pool.hpp"

void server_main_inner() {
  server_print("server lives");

  constexpr auto MYPORT = 3490u;

  ssh::Bind bind{MYPORT};

  bind.listen();

  std::atomic_bool server_running = true;
  BS::thread_pool pool(10);

  while (server_running.load()) {
    auto session_raw = bind.accept();
    auto session = std::make_shared<ssh::Session>(std::move(session_raw));

    pool.push_task([session = std::move(session)] {

    });
  }

  server_print("done");
}

void server_main() {
  try {
    server_main_inner();
  } catch (ssh::Error const& e) {
    server_print("Error: {}", e.what());
  }
}
