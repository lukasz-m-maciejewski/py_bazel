#include "connectivity_experiments/ssh_socket/client_main.hpp"

#include <libssh/libssh.h>
#include <libssh/libssh_version.h>

#include "connectivity_experiments/ssh_socket/ssh.hpp"

#include <array>
#include <stdexcept>
#include <string_view>

#include "connectivity_experiments/ssh_socket/client_print.hpp"

void client_main_inner() {
  client_print("client lives");
  ssh::Session session{"localhost", 3491};

  session.connect();

  session.verify_host();

  session.userauth("");

  {
    auto channel = session.make_channel();

    channel.open_session();

    std::string const msg = "Howdy Partner!\n";
    channel.write(msg);

    std::string const resp = channel.read();

    channel.close();
  }

  session.disconnect();
}

void client_main() {
  try {
    client_main_inner();
  } catch (ssh::Error const& e) {
    client_print("Exception caught: {}", e.what());
  }
}
