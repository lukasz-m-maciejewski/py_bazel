""" Stuff """

cc_binary(
    name = "ssh_socket",
    srcs = ["main.cpp"],
    deps = [
        ":client_main",
        ":server_main",
        "@fmt",
    ],
)

cc_library(
    name = "safe_print",
    hdrs = ["safe_print.hpp"],
    deps = [
        "@fmt",
    ],
)

cc_library(
    name = "server_print",
    hdrs = ["server_print.hpp"],
    deps = [
        ":safe_print",
    ],
)

cc_library(
    name = "client_print",
    hdrs = ["client_print.hpp"],
    deps = [
        ":safe_print",
    ],
)

cc_library(
    name = "server_main",
    srcs = ["server_main.cpp"],
    hdrs = ["server_main.hpp"],
    deps = [
        ":server_print",
        ":ssh",
        "//third_party/thread_pool",
    ],
)

cc_library(
    name = "client_main",
    srcs = ["client_main.cpp"],
    hdrs = ["client_main.hpp"],
    deps = [
        ":client_print",
        ":ssh",
        "//sys:libssh",
    ],
)

cc_library(
    name = "ssh",
    srcs = ["ssh.cpp"],
    hdrs = ["ssh.hpp"],
    deps = [
        "//sys:libssh",
        "@fmt",
    ],
)
