#pragma once

#include <fmt/core.h>
#include <libssh/callbacks.h>
#include <libssh/libssh.h>
#include <libssh/server.h>

#include <cstdint>
#include <stdexcept>
#include <string>

namespace ssh {

class Error : public std::runtime_error {
 public:
  Error(char const* msg) : std::runtime_error(msg) {}
  Error(std::string const& msg) : std::runtime_error(msg) {}
};

class Session;

class Channel {
  ssh_channel handle_;

  friend class Session;

  Channel(ssh_channel handle) : handle_{handle} {}

 public:
  Channel(Channel const&) = delete;
  auto operator=(Channel const&) -> Channel& = delete;

  Channel(Channel&&) = default;
  auto operator=(Channel&&) -> Channel& = default;

  void open_session() {
    auto rc = ssh_channel_open_session(handle_);
    if (rc != SSH_OK) {
      throw Error("ssh_channel_open_session");
    }
  }

  void close() {
    ssh_channel_close(handle_);
    ssh_channel_send_eof(handle_);
  }

  void write(std::string_view msg) {
    auto numWrites = ssh_channel_write(handle_, msg.data(), msg.size());
    if (static_cast<std::size_t>(numWrites) != msg.size()) {
      throw Error(fmt::format(
          "ssh_channel_write numWrites mismatch - expected:{} actual:{}",
          msg.size(), numWrites));
    }
  }

  auto read() -> std::string {
    constexpr auto buffer_size = 100u;
    std::string buffer{buffer_size, '\0'};

    auto num_read =
        ssh_channel_read(handle_, std::addressof(buffer[0]), buffer_size, 0);

    buffer.resize(num_read + 1);
    return buffer;
  }

  ~Channel() { ssh_channel_free(handle_); }
};

class Bind;

class Session final {
  ssh_session handle_;

  friend class Bind;

  Session(ssh_session handle) : handle_{handle} {}

 public:
  Session(Session const&) = delete;
  auto operator=(Session const&) -> Session& = delete;

  Session(Session&&) = default;
  auto operator=(Session&&) -> Session& = default;

  Session(char const* hostname, std::uint32_t port) : handle_{ssh_new()} {
    if (handle_ == nullptr) {
      throw Error("ssh_new failed");
    }

    ssh_options_set(handle_, SSH_OPTIONS_HOST, hostname);
    ssh_options_set(handle_, SSH_OPTIONS_PORT, &port);
  }

  void connect() {
    auto rc = ssh_connect(handle_);
    if (rc != SSH_OK) {
      throw Error("ssh_connect failed");
    }
  }

  void verify_host() {
    ssh_key srv_pubkey = nullptr;

    auto rc = ssh_get_server_publickey(handle_, &srv_pubkey);
    if (rc < 0) {
      throw Error("ssh_get_server_publickey");
    }

    std::uint8_t* hash = nullptr;
    std::size_t hash_len{};

    rc = ssh_get_publickey_hash(srv_pubkey, SSH_PUBLICKEY_HASH_SHA1, &hash,
                                &hash_len);
    ssh_key_free(srv_pubkey);
    if (rc < 0) {
      throw Error("ssh_get_publickey_hash");
    }

    ssh_known_hosts_e const state = ssh_session_is_known_server(handle_);

    switch (state) {
      case SSH_KNOWN_HOSTS_OK:
        // client_print("SSH_KNOWN_HOSTS_OK");
        break;
      case SSH_KNOWN_HOSTS_CHANGED:
        // client_print("SSH_KNOWN_HOSTS_CHANGED");
        break;
      case SSH_KNOWN_HOSTS_OTHER:
        // client_print("SSH_KNOWN_HOSTS_OTHER");
        break;
      case SSH_KNOWN_HOSTS_NOT_FOUND:
        // client_print("SSH_KNOWN_HOSTS_NOT_FOUND - fallthrough");
      case SSH_KNOWN_HOSTS_UNKNOWN:
        // client_print("SSH_KNOWN_HOSTS_UNKNOWN");
        break;
      case SSH_KNOWN_HOSTS_ERROR:
        // client_print("SSH_KNOWN_HOSTS_ERROR");
        break;
    }

    ssh_clean_pubkey_hash(&hash);
  }

  void userauth(char const* password) {
    auto rc = ssh_userauth_password(handle_, nullptr, password);
    if (rc != SSH_AUTH_SUCCESS) {
      throw Error(fmt::format(
          "ssh_userauth_password: Error authenticating with password: {}",
          ssh_get_error(handle_)));
    }
  }

  auto make_channel() -> Channel {
    auto channel_handle = ssh_channel_new(handle_);
    if (channel_handle == nullptr) {
      throw Error("ssh_channel_new");
    }
    return Channel{channel_handle};
  }

  void disconnect() { ssh_disconnect(handle_); }

  ~Session() { ssh_free(handle_); }

  void set_auth_methods(int auth_methods) {
    ssh_set_auth_methods(handle_, auth_methods);
  }

  void set_server_callbacks(::ssh_server_callbacks_struct* callbacks) {
    ssh_set_server_callbacks(handle_, callbacks);
  }
};

class Bind final {
  ssh_bind handle_;

 public:
  Bind(Bind const&) = delete;
  auto operator=(Bind const&) -> Bind& = delete;

  Bind(Bind&&) = default;
  auto operator=(Bind&&) -> Bind& = default;

  Bind(std::uint32_t port) : handle_{} {
    handle_ = ssh_bind_new();
    if (handle_ != nullptr) {
      throw Error{"failed to allocate ssh::Bind"};
    }

    ssh_bind_options_set(handle_, SSH_BIND_OPTIONS_BINDPORT,
                         std::addressof(port));
  }

  ~Bind() {
    if (handle_ != nullptr) {
      ssh_bind_free(handle_);
    }
  }

  void listen() {
    auto const rc = ssh_bind_listen(handle_);
    if (rc < 0) {
      throw Error(fmt::format("ssh_bind_listen: {}", ssh_get_error(handle_)));
    }
  }

  Session accept() {
    auto session = ssh_new();
    if (session == nullptr) {
      throw Error("failed to allocate session");
    }

    auto const rc = ssh_bind_accept(handle_, session);

    if (rc == SSH_ERROR) {
      throw Error(fmt::format("ssh_bind_accept: {}", ssh_get_error(handle_)));
    }

    return Session{session};
  }


};

class Event final {
  ssh_event handle_;

  struct EventData {
    ssh_channel channel = nullptr;
    bool authenticated = false;
    int auth_attempts = 0;
  };

  EventData data_;

 public:
  Event() : handle_{ssh_event_new()} {
    if (handle_ == nullptr) {
      throw Error("Could not create polling context");
    }
  }

  Event(Event const&) = delete;
  auto operator=(Event const&) -> Event& = delete;
  Event(Event&&) = default;
  auto operator=(Event&&) -> Event& = default;

  static ssh_channel channel_open_cb(ssh_session session, void* userdata) {
    auto data = static_cast<EventData*>(userdata);
    data->channel = ssh_channel_new(session);
    return data->channel;
  }

  static int auth_password_cb(ssh_session session, char const* user,
                              char const* passwd, void* userdata) {
    auto data = static_cast<EventData*>(userdata);
    std::string_view const password{passwd};
    if (password == "") {
      data->authenticated = true;
      return SSH_AUTH_SUCCESS;
    }

    data->auth_attempts += 1;
    return SSH_AUTH_DENIED;
  }

  ::ssh_channel_callbacks_struct make_channel_callbacks() { return {}; }

  ::ssh_server_callbacks_struct make_server_callbacks() {
    ::ssh_server_callbacks_struct callbacks{
        .userdata = static_cast<void*>(std::addressof(data_)),
        .auth_password_function = std::addressof(auth_password_cb)),
        .channel_open_request_session_function =
            std::addressof(channel_open_cb),
    };

    ssh_callbacks_init(std::addressof(callbacks));
    return callbacks;
  }

  void handle_session(Session& session) {
    auto server_cb = make_server_callbacks();

    session.set_auth_methods(SSH_AUTH_METHOD_PASSWORD);
    session.set_server_callbacks(&server_cb);

    auto rc = ssh_event_add_session(handle_, session.handle_);
    if (rc != SSH_OK) {
      throw Error("ssh_event_add_session");
    }
  }

  ~Event() { ssh_event_free(handle_); }
};

}  // namespace ssh
